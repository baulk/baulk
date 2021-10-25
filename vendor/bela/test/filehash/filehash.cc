//

#include <bela/terminal.hpp>
#include <bela/hash.hpp>

int wmain(int argc, wchar_t **argv) {
  if (argc < 2) {
    bela::FPrintF(stderr, L"usage: %s file\n", argv[0]);
    return 1;
  }
  bela::hash::sha256::Hasher h1;
  h1.Initialize(bela::hash::sha256::HashBits::SHA224);
  bela::hash::sha256::Hasher h2;
  h2.Initialize();
  bela::hash::sha512::Hasher h3;
  h3.Initialize(bela::hash::sha512::HashBits::SHA384);
  bela::hash::sha512::Hasher h4;
  h4.Initialize();
  bela::hash::sha3::Hasher h5;
  h5.Initialize(bela::hash::sha3::HashBits::SHA3224);
  bela::hash::sha3::Hasher h6;
  h6.Initialize();
  bela::hash::sha3::Hasher h7;
  h7.Initialize(bela::hash::sha3::HashBits::SHA3384);
  bela::hash::sha3::Hasher h8;
  h8.Initialize(bela::hash::sha3::HashBits::SHA3512);
  bela::hash::blake3::Hasher h9;
  h9.Initialize();
  bela::hash::sm3::Hasher h10;
  h10.Initialize();
  FILE *fd = nullptr;
  if (auto e = _wfopen_s(&fd, argv[1], L"rb"); e != 0) {
    auto ec = bela::make_stdc_error_code(e);
    bela::FPrintF(stderr, L"unable open file: %s\n", ec);
    return 1;
  }
  auto closer = bela::finally([&] { fclose(fd); });
  char buffer[32678];
  for (;;) {
    auto n = fread(buffer, 1, sizeof(buffer), fd);
    h1.Update(buffer, n);
    h2.Update(buffer, n);
    h3.Update(buffer, n);
    h4.Update(buffer, n);
    h5.Update(buffer, n);
    h6.Update(buffer, n);
    h7.Update(buffer, n);
    h8.Update(buffer, n);
    h9.Update(buffer, n);
    h10.Update(buffer, n);
    if (n < sizeof(buffer)) {
      break;
    }
  }
  bela::FPrintF(stdout, L"SHA224: %s\n", h1.Finalize());
  bela::FPrintF(stdout, L"SHA256: %s\n", h2.Finalize());
  bela::FPrintF(stdout, L"SHA384: %s\n", h3.Finalize());
  bela::FPrintF(stdout, L"SHA512: %s\n", h4.Finalize());
  bela::FPrintF(stdout, L"SHA3-224: %s\n", h5.Finalize());
  bela::FPrintF(stdout, L"SHA3-256: %s\n", h6.Finalize());
  bela::FPrintF(stdout, L"SHA3-384: %s\n", h7.Finalize());
  bela::FPrintF(stdout, L"SHA3-512: %s\n", h8.Finalize());
  bela::FPrintF(stdout, L"BLAKE3: %s\n", h9.Finalize());
  bela::FPrintF(stdout, L"SM3: %s\n", h10.Finalize());
  return 0;
}