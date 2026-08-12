#pragma once

#include <list>
#include <string>
#include <utility>

#include <openssl/ssl.h>

#include "types.h"

namespace Core {

/* Most OpenSSL handling resides in this class. */
class SSLManager {
public:
  enum class KeyAlgorithm { EC_prime256v1 = 0, SLH_DSA_SHA2_128s = 1, ML_DSA_44 = 2 };

  /* List all the loaded certificate/private keys as binary data in DER
   * format.
   */
  static std::list<std::pair<BinaryData, BinaryData>> certKeyPairs();

  /* Load a cert/key pair into OpenSSL by providing binary data.
   * The data can be either in PEM or DER format. */
  static bool addCertKeyPair(const BinaryData& key, const BinaryData& cert);

  /* Clear all loaded cert/key pairs */
  static void clearCertKeyPairs();

  /* Must be called by every thread intending to issue OpenSSL commands */
  static void init();

  /* When linking OpenSSL statically, this must be called by every thread
   * that has issued OpenSSL commands
   */
  static void cleanupThread();

  /* Free global OpenSSL resources */
  static void cleanup();

  static SSL_CTX* getClientSSLCTX();
  static SSL_CTX* getServerSSLCTX();
  static bool addCertKeyPair(EVP_PKEY* pkey, X509* x509);
  static const char* getCipher(SSL* ssl);
  static std::string version();
  static std::string sslErrorToString(int error);
  static bool isAbruptDisconnectError(unsigned long e);

  static KeyAlgorithm getKeyAlgorithm();
  static void setKeyAlgorithm(KeyAlgorithm algo);
  static bool isKeyAlgorithmAvailable(KeyAlgorithm algo);

private:
  static KeyAlgorithm keyalgorithm;
};

} // namespace Core
