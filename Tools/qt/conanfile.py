from conans import ConanFile, CMake, tools


class QtWebKitConan(ConanFile):
    name = "qtwebkit"
    version = "5.212.0-alpha4"
    license = "LGPL-2.0-or-later, LGPL-2.1-or-later, BSD-2-Clause"
    url = "https://github.com/qtwebkit/qtwebkit"
    description = "Qt port of WebKit"
    topics = ( "qt", "browser-engine", "webkit", "qt5", "qml", "qtwebkit" )
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    exports_sources = "../../*"
    no_copy_source = True
    options = {
        "use_ccache": [True, False],
        "qt5_dir": "ANY"
    }
    requires = (
        "icu/65.1@qtproject/stable",
        "libxml2/2.9.10@qtproject/stable",
        "libxslt/1.1.34@qtproject/stable",
        "libjpeg-turbo/2.0.3@qtproject/stable",
        "zlib/1.2.11",

        "libpng/1.6.37",
        "sqlite3/3.31.1"
    )
    build_requires = (
        "ninja/1.9.0",
        "cmake/3.16.4"
    )
    default_options = {
        "icu:shared": True,
        "icu:data_packaging": "library",

        "libxml2:shared": True,
        "libxml2:iconv": False,
        "libxml2:icu": True,
        "libxml2:zlib": False,

        "libxslt:shared": True,

        "libjpeg-turbo:shared": False
    }

    def build_requirements(self):
        if self.settings.os == 'Linux':
            if not tools.which('pkg-config'):
                self.build_requires('pkg-config_installer/0.29.2@bincrafters/stable')

        # gperf python perl bison ruby flex
        if not tools.which("gperf"):
            self.build_requires("gperf_installer/3.1@conan/stable")
        if not tools.which("perl"):
            self.build_requires("strawberryperl/5.30.0.1")
        if not tools.which("ruby"):
            self.build_requires("ruby_installer/2.6.3@bincrafters/stable")
        if not tools.which("bison"):
            self.build_requires("bison_installer/3.3.2@bincrafters/stable")
        if not tools.which("flex"):
            self.build_requires("flex_installer/2.6.4@bincrafters/stable")

    def build(self):
        cmake = CMake(self)
        cmake.generator = "Ninja"

        if self.options.use_ccache:
            cmake.definitions["CMAKE_C_COMPILER_LAUNCHER"] = "ccache"
            cmake.definitions["CMAKE_CXX_COMPILER_LAUNCHER"] = "ccache"

        if self.options.qt5_dir:
            cmake.definitions["Qt5_DIR"] = self.options.qt5_dir

        print(self.source_folder)
        print()
        print(self.build_folder)

        cmake.configure()
        cmake.build(args="-v")

    def package(self):
        pass

    def package_info(self):
        pass
