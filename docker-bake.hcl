variable "HOST_OS" {
  # default if caller doesn't export HOST_OS
  default = "linux"
}

target "gcc9" {
  dockerfile = "Dockerfile.linux"
  target     = "gcc9"
  tags       = ["toolchain:gcc9"]
}

target "clang11" {
  dockerfile = "Dockerfile.linux"
  target     = "clang11"
  tags       = ["toolchain:clang11"]
}

target "gcc-modern" {
  dockerfile = "Dockerfile.linux"
  target     = "gcc-modern"
  tags       = ["toolchain:gcc-modern"]
}

target "clang-modern" {
  dockerfile = "Dockerfile.linux"
  target     = "clang-modern"
  tags       = ["toolchain:clang-modern"]
}

target "msvc-clang-cl" {
  dockerfile = "Dockerfile.windows"
  target     = "msvc-clang-cl"
  tags       = ["toolchain:msvc-clang-cl"]
}

group "default" {
  targets = HOST_OS == "windows" ? ["msvc-clang-cl"] : ["gcc9", "clang11", "gcc-modern", "clang-modern"]
}