from conan import ConanFile
from conan.tools.cmake import cmake_layout


class CoinsDbConan(ConanFile):
    """Conan recipe for the coins-db project.

    Provides the C++ dependencies for the shared core library, the CLI, the
    REST server, and the GoogleTest-based test suite.
    """

    name = "coins-db"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"

    # Consume dependencies and generate the CMake toolchain + find configs.
    generators = "CMakeToolchain", "CMakeDeps"

    # Enable RapidCheck's GoogleTest integration (RC_GTEST_PROP). It is off by
    # default in the ConanCenter recipe.
    default_options = {"rapidcheck/*:enable_gtest": True}

    def requirements(self):
        # Core storage.
        self.requires("sqlite3/3.53.4")
        # JSON for the REST API and import/export.
        self.requires("nlohmann_json/3.12.0")
        # Lightweight HTTP server for coins_server.
        self.requires("cpp-httplib/0.56.0")
        # Command-line parsing for coins_cli.
        self.requires("cli11/2.6.2")

    def build_requirements(self):
        # GoogleTest is the project's testing framework (test-only dependency).
        # force=True so our version also wins over RapidCheck's older transitive
        # gtest pin when its GoogleTest integration is enabled.
        self.test_requires("gtest/1.17.0", force=True)
        # RapidCheck: property-based testing (QuickCheck-style), test-only.
        self.test_requires("rapidcheck/cci.20231215")

    def layout(self):
        cmake_layout(self)
