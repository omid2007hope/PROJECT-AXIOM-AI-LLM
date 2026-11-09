from importlib.metadata import PackageNotFoundError, version as _pkg_version

try:
    __version__ = _pkg_version("openosint")
except PackageNotFoundError:
    __version__ = "unknown"

version = __version__
