import warnings
from importlib.metadata import version, PackageNotFoundError

warnings.filterwarnings(
    action="ignore",
    message="The `squeeze` kwarg to GroupBy is being removed",
    category=UserWarning,
)

try:
    __version__ = version("spade")
except PackageNotFoundError:
    try:
        from mcnp_abaqus_tools import _version
        __version__ = _version.version
    except ImportError:
        # Should only hit this when running as an un-installed package in the local repository
        import pathlib
        warnings.filterwarnings(action='ignore', message='tag', category=UserWarning, module='setuptools_scm')
        import setuptools_scm
        __version__ = setuptools_scm.get_version(root=pathlib.Path(__file__).parent.parent)
        # Remove third-party packages from the project namespace
        del pathlib, setuptools_scm

# Remove third-party packages from the project namespace
del warnings, version, PackageNotFoundError