set -x
system_test_directory=$PWD
cd $SP_DIR/$PKG_NAME
pytest -n 4 -vvv -m "not require_third_party" --system-test-dir=${system_test_directory}
pytest -n 4 -v --no-showlocals -m "systemtest and require_third_party" --tb=short --system-test-dir=${system_test_directory}/abq2024 --abaqus-command=/apps/abaqus/Commands/abq2024
pytest -n 4 -v --no-showlocals -m "systemtest and require_third_party" --tb=short --system-test-dir=${system_test_directory}/abq2023 --abaqus-command=/apps/abaqus/Commands/abq2023
pytest -n 4 -v --no-showlocals -m "systemtest and require_third_party" --tb=short --system-test-dir=${system_test_directory}/abq2022 --abaqus-command=/apps/abaqus/Commands/abq2022
