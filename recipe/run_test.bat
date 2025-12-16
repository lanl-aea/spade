set system_test_directory=%CD%
cd %SP_DIR%\%PKG_NAME%
pytest -n 4 -vvv -m "not require_third_party" --system-test-dir=%system_test_directory%
