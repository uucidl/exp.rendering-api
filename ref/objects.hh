#pragma once

/**
 * Enforce identity semantics for this type name.
 *
 * an object with identity cannot be copied or moved, and must
 * instead be referenced to.
 */
#define ENFORCE_ID_OBJECT(Typename)                     \
        private:                                        \
        Typename(Typename&) = delete;                   \
        Typename(Typename&&) = delete;                  \
        Typename& operator=(Typename&) = delete;        \
        Typename& operator=(Typename&&) = delete;

#define ENFORCE_UNIQUE_REF_OBJECT(Typename)     \
  private: \
Typename(Typename&) = delete;                           \
        Typename& operator=(Typename&) = delete;
