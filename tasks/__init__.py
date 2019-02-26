from tasks.compile import (
    build_python_host,
    build_python,
    build_python_emscripten,
    clean_build,
    funcs,
    funcs_emscripten,
    compile_libfaasm,
    compile_libfaasm_emscripten,
    compile_eigen,
    compile_eigen_emscripten,
    compile_libcurl,
)

from tasks.docker import (
    build_all,
    build_base,
    build_data,
    build_edge,
    build_redis,
    build_worker,
    build_upload,
    build_toolchain,
    data,
    purge_images,
    push_all,
    push_data,
    push_edge,
    push_redis,
    push_upload,
    push_worker,
    push_toolchain,
    tools,
    pull,
    push_base
)

from tasks.docker_compose import start_all, stop_all, restart_all

from tasks.aws import (
    build_lambda_worker,
    build_lambda_codegen,
    build_lambda_dispatch,
    build_lambda_func,
    build_lambda_redis,
    upload_lambda_worker,
    upload_lambda_codegen,
    upload_lambda_dispatch,
    upload_lambda_func,
    upload_lambda_redis,
)

from tasks.upload import (
    upload_funcs
)

from tasks.wasm2wast import wast
