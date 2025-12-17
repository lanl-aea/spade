import argparse
import sys

from spade import __version__, _docs, _extract, _settings

_exclude_from_namespace = set(globals().keys())


def main() -> None:
    """Run the SPADE command line interface."""
    parser = get_parser()
    args = parser.parse_args()

    try:
        if args.subcommand == "docs":
            _docs.main(_settings._installed_docs_index, print_local_path=args.print_local_path)
        elif args.subcommand == "extract":
            _extract.main(args)
        else:
            parser.print_help()
    except RuntimeError as err:
        sys.exit(str(err))


def get_parser() -> argparse.ArgumentParser:
    """Get parser object for command line options.

    :return: parser
    :rtype: ArgumentParser
    """
    main_parser = argparse.ArgumentParser(
        description=_settings._project_description,
        prog=f"{_settings._project_name_short}",
    )

    main_parser.add_argument(
        "-V",
        "--version",
        action="version",
        version=f"{_settings._project_name_short} {__version__}",
    )

    subparsers = main_parser.add_subparsers(title="subcommands", metavar="{subcommand}", dest="subcommand")

    subparsers.add_parser(
        "docs",
        help=f"Open the {_settings._project_name_short.upper()} HTML documentation",
        description=(
            f"Open the packaged {_settings._project_name_short.upper()} HTML documentation in the "
            "system default web browser"
        ),
        parents=[_docs.get_parser()],
    )

    subparsers.add_parser(
        "extract",
        help="Extract ODB file to H5",
        parents=[_extract.get_parser()],
    )

    return main_parser


if __name__ == "__main__":
    main()  # pragma: no cover


# Limit help() and 'from module import *' behavior to the module's public API
_module_objects = set(globals().keys()) - _exclude_from_namespace
__all__ = [name for name in _module_objects if not name.startswith("_")]
