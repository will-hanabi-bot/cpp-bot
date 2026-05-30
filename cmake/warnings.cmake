function(hanabi_set_warnings target)
  target_compile_options(${target} PRIVATE
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
    -Wno-sign-compare
  )
endfunction()
