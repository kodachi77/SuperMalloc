group "default" {
  targets = ["gcc9", "clang11", "gcc-modern", "clang-modern"]
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
