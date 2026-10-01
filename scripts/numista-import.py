#!/usr/bin/env python3
"""Import a coin from Numista into coins-db via the CLI.

Given a Numista coin URL or type id, this fetches the coin's catalogue data from
the official Numista REST API v3 and runs ``coins_cli add`` to create a matching
entry. (Raw page scraping is not supported: Numista reliably blocks non-browser
clients with HTTP 403, so the API is the only workable path.)

Set ``NUMISTA_API_KEY`` to a key from your Numista account (Profile -> API).

Usage:
    export NUMISTA_API_KEY=...
    scripts/numista-import.py https://en.numista.com/10203
    scripts/numista-import.py 10203 --data-dir mydata
    scripts/numista-import.py 10203 --dry-run       # print the CLI command only
    scripts/numista-import.py 10203 --year 1895     # pin a specific coin year
    scripts/numista-import.py 10203 --obverse front.jpg --reverse back.jpg

Environment:
    NUMISTA_API_KEY   Numista API v3 key (required).
    COINS_CLI         Path to the coins_cli binary (overrides the default).
"""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, Optional

API_BASE = "https://api.numista.com/v3"


# --------------------------------------------------------------------------- #
# Input parsing
# --------------------------------------------------------------------------- #
def parse_type_id(url_or_id: str) -> int:
    """Extract the Numista type id from a URL or a bare id.

    Accepts e.g. ``10203``, ``https://en.numista.com/10203``, or a catalogue
    URL like ``https://en.numista.com/catalogue/pieces10203.html``.
    """
    s = url_or_id.strip()
    if s.isdigit():
        return int(s)
    # pieces<ID>.html form
    m = re.search(r"pieces(\d+)\.html", s)
    if m:
        return int(m.group(1))
    # bare numeric path segment, e.g. en.numista.com/10203
    m = re.search(r"numista\.com/(?:[a-z]+/)?(\d+)\b", s)
    if m:
        return int(m.group(1))
    # last-resort: any standalone run of digits
    m = re.search(r"(\d{3,})", s)
    if m:
        return int(m.group(1))
    raise ValueError(f"Could not find a Numista type id in: {url_or_id!r}")


def page_url(type_id: int) -> str:
    return f"https://en.numista.com/catalogue/pieces{type_id}.html"


# --------------------------------------------------------------------------- #
# Fetching
# --------------------------------------------------------------------------- #
def _get(url: str, headers: dict[str, str]) -> bytes:
    req = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(req, timeout=30) as resp:  # noqa: S310 (trusted host)
        return resp.read()


def fetch_api(type_id: int, api_key: str, lang: str = "en") -> dict[str, Any]:
    """Fetch a coin type from the official API. Raises on HTTP/parse error."""
    url = f"{API_BASE}/types/{type_id}?lang={lang}"
    headers = {"Numista-API-Key": api_key, "Accept": "application/json"}
    try:
        raw = _get(url, headers)
    except urllib.error.HTTPError as e:
        detail = ""
        try:
            detail = e.read().decode("utf-8", "replace")[:300]
        except Exception:  # noqa: BLE001
            pass
        if e.code == 401:
            raise SystemExit(
                "Numista API returned 401 Unauthorized. Check NUMISTA_API_KEY."
            ) from e
        if e.code == 404:
            raise SystemExit(f"Numista API: type {type_id} not found (404).") from e
        raise SystemExit(f"Numista API error {e.code}: {detail}") from e
    except urllib.error.URLError as e:
        raise SystemExit(f"Network error contacting Numista API: {e.reason}") from e
    return json.loads(raw)


# --------------------------------------------------------------------------- #
# Field mapping
# --------------------------------------------------------------------------- #
# A coin record destined for `coins_cli add`. Values are strings/None; the
# builder below turns them into CLI flags.
Fields = dict[str, Optional[str]]


def _num(value: Any) -> Optional[str]:
    """Render a numeric JSON value as a plain string, or None."""
    if value is None:
        return None
    if isinstance(value, (int, float)):
        # Drop a trailing .0 so 1.0 -> "1".
        return str(int(value)) if float(value).is_integer() else str(value)
    s = str(value).strip()
    return s or None


def _parse_value_text(text: Optional[str]) -> tuple[Optional[str], Optional[str]]:
    """Split Numista's ``value.text`` (e.g. ``"1 Cent"``, ``"½ Dollar"``,
    ``"50 Øre"``) into (amount, unit).

    Returns the amount as a plain decimal string and the unit (lower-cased,
    singular-ish) as the coin actually denominates it, so we can store "1 cent"
    rather than "0.01 dollar". Returns (None, None) when it can't parse.
    """
    if not text:
        return None, None
    s = text.strip()
    # Leading amount: integer/decimal, or a common vulgar fraction.
    fractions = {"½": "0.5", "¼": "0.25", "¾": "0.75", "⅓": "0.333", "⅔": "0.667",
                 "⅛": "0.125", "⅒": "0.1", "⅕": "0.2"}
    m = re.match(r"\s*([0-9]+(?:[.,][0-9]+)?)\s+(.*\S)\s*$", s)
    if m:
        amount = m.group(1).replace(",", ".")
        unit = m.group(2)
    else:
        mf = re.match(r"\s*([½¼¾⅓⅔⅛⅒⅕])\s+(.*\S)\s*$", s)
        if not mf:
            return None, None
        amount = fractions.get(mf.group(1))
        unit = mf.group(2)
    if amount is None or not unit:
        return None, None
    # Normalize the unit: drop a trailing plural "s", lower-case for the lookup
    # (coins-db treats units case-insensitively and resolves/creates by name).
    unit = unit.strip()
    singular = re.sub(r"s$", "", unit, flags=re.IGNORECASE) if len(unit) > 2 else unit
    return _num(amount), singular.lower()


def map_api(data: dict[str, Any], type_id: int) -> Fields:
    """Map a `GET /types/{id}` payload to coins-db fields."""
    issuer = data.get("issuer") or {}
    value = data.get("value") or {}
    currency = value.get("currency") or {}
    composition = data.get("composition") or {}

    # Numista exposes year bounds as min_year/max_year (issue span of the type).
    year = _num(data.get("min_year"))
    year_to = _num(data.get("max_year"))

    currency_name = currency.get("full_name") or currency.get("name") or None

    # Face value: prefer the coin's own denomination from `value.text`
    # (e.g. "1 Cent" -> 1 + unit "cent") over `numeric_value`, which Numista
    # expresses in the *major* unit (e.g. 0.01 dollar for a 1-cent coin).
    face_value, face_unit = _parse_value_text(value.get("text"))
    if face_value is None:
        face_value = _num(value.get("numeric_value"))
        face_unit = None  # fall back to the currency's major unit
    else:
        # If the parsed unit is really just the major currency name, let the CLI
        # default to the major unit instead of creating a redundant sub-unit.
        major = (currency.get("name") or "").strip().lower()
        if face_unit and major and face_unit.rstrip("s") == major.rstrip("s"):
            face_unit = None

    title = (data.get("title") or "").strip() or None
    fields: Fields = {
        "country": (issuer.get("name") or None),
        "year": year,
        "year-to": year_to if (year_to and year_to != year) else None,
        # value.text is the human denomination, e.g. "1 Dollar" / "1 Cent".
        "denomination": (value.get("text") or title or None),
        "face-value": face_value,
        "face-unit": face_unit,
        "currency": currency_name,
        "composition": (composition.get("text") or None),
        "weight": _num(data.get("weight")),
        "diameter": _num(data.get("size")),  # Numista "size" is the diameter (mm).
        # The coin title (if any) goes to notes; the Numista URL goes to a
        # reference link instead (added after the coin is created).
        "notes": title,
        # Not CLI `add` flags — consumed separately to create a reference link.
        "_link_url": page_url(type_id),
        "_link_label": f"Numista — {title}" if title else f"Numista {type_id}",
    }
    return fields


# --------------------------------------------------------------------------- #
# CLI invocation
# --------------------------------------------------------------------------- #
def default_cli_path() -> Path:
    env = os.environ.get("COINS_CLI")
    if env:
        return Path(env)
    root = Path(__file__).resolve().parent.parent
    return root / "build" / "build" / "Debug" / "cli" / "coins_cli"


def build_add_args(fields: Fields) -> list[str]:
    """Turn the mapped fields into `coins_cli add` flags (skipping empties)."""
    args = ["add"]
    for flag in (
        "country",
        "year",
        "year-to",
        "denomination",
        "face-value",
        "face-unit",
        "currency",
        "composition",
        "weight",
        "diameter",
        "notes",
    ):
        val = fields.get(flag)
        if val is not None and val != "":
            args += [f"--{flag}", val]
    return args


def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="Import a coin from Numista into coins-db via the CLI.",
    )
    parser.add_argument("url_or_id", help="Numista coin URL or type id (e.g. 10203)")
    parser.add_argument(
        "--year",
        type=int,
        help="Specific year of your coin, overriding Numista's type year range "
        "(also clears the year-to range). Warns if outside the catalogued span.",
    )
    parser.add_argument(
        "--data-dir", help="coins-db data directory (passed to coins_cli --data-dir)"
    )
    parser.add_argument(
        "--obverse",
        metavar="FILE",
        help="Photo of your coin's obverse (front) to attach, from a local file",
    )
    parser.add_argument(
        "--reverse",
        metavar="FILE",
        help="Photo of your coin's reverse (back) to attach, from a local file",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the coins_cli command instead of running it",
    )
    parser.add_argument("--lang", default="en", help="API language (en, es, fr)")
    args = parser.parse_args(argv)

    # Validate image paths up front so we fail before creating a coin.
    images: list[tuple[str, str]] = []  # (kind, path)
    for kind, path in (("obverse", args.obverse), ("reverse", args.reverse)):
        if path:
            if not Path(path).is_file():
                print(f"--{kind} image not found: {path}", file=sys.stderr)
                return 2
            images.append((kind, path))

    try:
        type_id = parse_type_id(args.url_or_id)
    except ValueError as e:
        print(str(e), file=sys.stderr)
        return 2

    api_key = os.environ.get("NUMISTA_API_KEY")
    if not api_key:
        print(
            "NUMISTA_API_KEY is not set. Get a free key from your Numista account "
            "(Profile → API) and export it, e.g.:\n"
            "    export NUMISTA_API_KEY=your-key-here",
            file=sys.stderr,
        )
        return 2

    data = fetch_api(type_id, api_key, args.lang)
    fields = map_api(data, type_id)
    source = "Numista API v3"

    # A specific coin has one year; let the user pin it, overriding Numista's
    # type-level range. Warn (don't fail) if it falls outside the catalogued span.
    if args.year is not None:
        span_lo = fields.get("year")
        span_hi = fields.get("year-to") or span_lo
        try:
            lo = int(span_lo) if span_lo else None
            hi = int(span_hi) if span_hi else None
        except (TypeError, ValueError):
            lo = hi = None
        if lo is not None and hi is not None and not (lo <= args.year <= hi):
            print(
                f"Warning: --year {args.year} is outside Numista's catalogued "
                f"range {lo}–{hi} for this type. Using {args.year} anyway.",
                file=sys.stderr,
            )
        fields["year"] = str(args.year)
        fields["year-to"] = None  # a specific coin is a single year, not a range

    # Country + at least one year are required by coins-db.
    missing = [k for k in ("country", "year") if not fields.get(k)]
    if missing:
        print(
            f"Could not determine required field(s) {missing} from {source}. "
            "The coin was not added; add it manually or supply the year with --year.",
            file=sys.stderr,
        )
        # Still show what we did extract, to help manual entry.
        print(json.dumps({k: v for k, v in fields.items() if v}, indent=2), file=sys.stderr)
        return 1

    cli = default_cli_path()
    prefix = [str(cli)]
    if args.data_dir:
        prefix += ["--data-dir", args.data_dir]
    add_cmd = prefix + build_add_args(fields)

    link_url = fields.get("_link_url")
    link_label = fields.get("_link_label")

    if args.dry_run:
        print(f"# source: {source}")
        print(" ".join(_shquote(c) for c in add_cmd))
        if link_url:
            # The coin id isn't known until `add` runs; show the shape.
            link_cmd = prefix + [
                "link", "add", "<coin_id>", "--label", link_label or "Numista",
                "--url", link_url,
            ]
            print(" ".join(_shquote(c) for c in link_cmd))
        for kind, path in images:
            img_cmd = prefix + ["image", "add", "<coin_id>", "--file", path, "--kind", kind]
            print(" ".join(_shquote(c) for c in img_cmd))
        return 0

    if not cli.exists():
        print(
            f"coins_cli not found at: {cli}\n"
            "Build the project first, or set COINS_CLI to the binary path.",
            file=sys.stderr,
        )
        return 1

    print(f"Adding coin from {source} (type {type_id})…", file=sys.stderr)
    add = subprocess.run(add_cmd, check=False, capture_output=True, text=True)
    # Mirror the CLI's own output to the user.
    if add.stdout:
        sys.stdout.write(add.stdout)
    if add.stderr:
        sys.stderr.write(add.stderr)
    if add.returncode != 0:
        return add.returncode

    # Attach the Numista URL as a reference link on the new coin.
    coin_id = _parse_added_id(add.stdout)
    if link_url and coin_id is not None:
        link_cmd = prefix + [
            "link", "add", str(coin_id),
            "--label", link_label or f"Numista {type_id}",
            "--url", link_url,
        ]
        link = subprocess.run(link_cmd, check=False)
        if link.returncode != 0:
            print(
                f"Coin {coin_id} was added, but attaching the Numista link failed. "
                f"Add it manually:\n    {' '.join(_shquote(c) for c in link_cmd)}",
                file=sys.stderr,
            )
            return link.returncode
    elif link_url and coin_id is None:
        print(
            "Coin was added, but its id could not be parsed from the CLI output, "
            f"so the Numista link was not attached. Add it manually: {link_url}",
            file=sys.stderr,
        )

    # Attach obverse/reverse images (CLI copies the files into the store).
    if images and coin_id is None:
        print(
            "Coin was added, but its id could not be parsed, so images were not "
            "attached. Add them manually with `coins image add`.",
            file=sys.stderr,
        )
    else:
        for kind, path in images:
            img_cmd = prefix + [
                "image", "add", str(coin_id), "--file", path, "--kind", kind,
            ]
            img = subprocess.run(img_cmd, check=False)
            if img.returncode != 0:
                print(
                    f"Coin {coin_id} was added, but attaching the {kind} image failed. "
                    f"Add it manually:\n    {' '.join(_shquote(c) for c in img_cmd)}",
                    file=sys.stderr,
                )
                return img.returncode

    return 0


def _parse_added_id(stdout: Optional[str]) -> Optional[int]:
    """Extract the new coin id from `coins add` output ('Added coin <id>')."""
    if not stdout:
        return None
    m = re.search(r"Added coin\s+(\d+)", stdout)
    return int(m.group(1)) if m else None


def _shquote(s: str) -> str:
    """Minimal shell quoting for a readable, copy-pasteable dry-run line."""
    if s and re.fullmatch(r"[A-Za-z0-9_./=:-]+", s):
        return s
    return "'" + s.replace("'", "'\\''") + "'"


if __name__ == "__main__":
    raise SystemExit(main())
