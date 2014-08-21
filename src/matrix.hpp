#pragma once

#include <math.h>

#define FLOAT_EPSILON (1e-6f)

/**
 * Compare whether two floats are close to one another.
 */
static inline int almost_equal (float const f1, float const f2,
                                float const tolerance)
{
        if (fabs(f1 - f2) < tolerance) {
                return 1;
        } else if (fabs(f2) > fabs(f1)) {
                return fabs((f2 - f1) / f2) < tolerance;
        } else {
                return fabs((f1 - f2) / f1) < tolerance;
        }
}

/**
 * Clamp a floating point number between two boundaries.
 */
static inline float clamp_f (float const f, float const min_b,
                             float const max_b)
{
        return f < min_b ? min_b : (f > max_b ? max_b : f);
}

/* 1- VECTORS */

/**
 * @class vector4
 *
 * A type representing xyzw coordinates.
 *
 * vector4 (4 coordinates vectors) are useful in 3d mesh as they
 * represent the 3 (x/y/z) vector space coordinates with extra room to
 * store the projective space w coordinate.
 *
 * vector4 are also a convenient representation for argb colors.
 */
typedef float vector4[4];

/**
 * Initializes a 4d vector.
 */
static inline void vector4_make (vector4 v, float x, float y, float z,
                                 float w)
{
        v[0] = x;
        v[1] = y;
        v[2] = z;
        v[3] = w;
}

/**
 * Build a 3d vector. The projective space coordinate is defaulted to 1.0
 */
static inline void vector4_make3(vector4 v, float x, float y, float z)
{
        vector4_make(v, x, y, z, 1.0f);
}


/**
 * Or how to fill a 4d verctor from 2d info.
 */
static inline void vector4_make2 (vector4 v, float x, float y)
{
        vector4_make3(v, x, y, 0.0f);
}


static inline void vector4_null (vector4 v)
{
        vector4_make(v, 0.0, 0.0, 0.0, 0.0);
}

static inline void vector4_null3 (vector4 v)
{
        vector4_make3(v, 0.0, 0.0, 0.0);
}


/**
 * Copy the source vector over the destination.
 */
static inline void vector4_copy (vector4 dst, vector4 const src)
{
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
}

static inline void vector4_sub (vector4 dst_a, vector4 b)
{
        dst_a[0] -= b[0];
        dst_a[1] -= b[1];
        dst_a[2] -= b[2];
        dst_a[3] -= b[3];
}

static inline void vector4_add (vector4 dst_a, vector4 b)
{
        dst_a[0] += b[0];
        dst_a[1] += b[1];
        dst_a[2] += b[2];
        dst_a[3] += b[3];
}

static inline void vector4_addscale (vector4 dst_a, vector4 b, float scale)
{
        dst_a[0] += scale * b[0];
        dst_a[1] += scale * b[1];
        dst_a[2] += scale * b[2];
        dst_a[3] += scale * b[3];
}

static inline void vector4_sub3 (vector4 dst_a, vector4 b)
{
        dst_a[0] -= b[0];
        dst_a[1] -= b[1];
        dst_a[2] -= b[2];
}

static inline void vector4_add3(vector4 dst_a, vector4 b)
{
        dst_a[0] += b[0];
        dst_a[1] += b[1];
        dst_a[2] += b[2];
}

static inline void vector4_addscale3 (vector4 dst_a, vector4 b, float scale)
{
        dst_a[0] += scale * b[0];
        dst_a[1] += scale * b[1];
        dst_a[2] += scale * b[2];
}

/**
 * Scale a vector
 */
static inline void vector4_scale (vector4 self, float b)
{
        self[0] *= b;
        self[1] *= b;
        self[2] *= b;
        self[3] *= b;
}

/**
 * Same in 3d
 */
static inline void vector4_scale3 (vector4 self, float b)
{
        self[0] *= b;
        self[1] *= b;
        self[2] *= b;
}

/**
 * Dot (scalar) product of two vectors
 */
static inline float vector4_dot(vector4 a, vector4 b)
{
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2] + a[3]*b[3];
}

/**
 * Dot (scalar) product of two vectors
 */
static inline float vector4_dot3(vector4 a, vector4 b)
{
        return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

/**
 * cross product between two vectors
 */
static inline void vector4_cross (vector4 dest, vector4 a, vector4 b)
{
        vector4_make3(dest,
                      +(a[1]*b[2] - a[2]*b[1]),
                      -(a[0]*b[2] - a[2]*b[0]),
                      +(a[0]*b[1] - a[1]*b[0]));
}

static inline void vector4_clamp (vector4 dest, float lowest, float highest)
{
        int i;
        for (i = 0; i < 4; i++) {
                dest[i] = clamp_f(dest[i], lowest, highest);
        }
}

static inline void vector4_clamp3 (vector4 dest, float lowest, float highest)
{
        int i;
        for (i = 0; i < 3; i++) {
                dest[i] = clamp_f(dest[i], lowest, highest);
        }
}

/**
 * Returns the norm of a vector
 */
static inline float vector4_norm (vector4 self)
{
        return sqrtf (vector4_dot(self, self));
}

/**
 * Same in 3d
 */
static inline float vector4_norm3 (vector4 self)
{
        return sqrtf (vector4_dot3(self, self));
}


/**
 * Returns the square of the norm.
 */
static inline float vector4_sqrnorm (vector4 self)
{
        return vector4_dot(self, self);
}

/**
 * Same in 3d
 */
static inline float vector4_sqrnorm3 (vector4 self)
{
        return vector4_dot3(self, self);
}

/**
 * Equality between two vectors
 */
static inline int vector4_equals (vector4 a, vector4 b)
{
        return almost_equal(a[0], b[0], FLOAT_EPSILON) &&
               almost_equal(a[1], b[1], FLOAT_EPSILON) &&
               almost_equal(a[2], b[2], FLOAT_EPSILON) &&
               almost_equal(a[3], b[3], FLOAT_EPSILON);
}

/**
 * Obtain the vector v, projected on 'onto'
 */
static inline void vector4_project3 (vector4 dest, vector4 onto, vector4 v)
{
        vector4_copy(dest, onto);
        vector4_scale3(dest, 1.0f / vector4_norm3(dest));

        {
                float m = vector4_dot3(v, dest);

                vector4_scale3(dest, m);
        }
}

/* 1.2 Coordinate systems */

/**
 * Converts a cartesian coordinate vector (standard representation) into a spherical coordinate vector.
 *
 * @param dest destination vector
 * @param cartesian source vector
 */
static inline void cartesian_to_spherical (vector4 dest, vector4 cartesian)
{
        float norm = vector4_norm3(cartesian);
        if (norm > 0.0f) {
                vector4_make(dest,
                             norm,
                             atan2f(cartesian[1], cartesian[0]),
                             acosf(cartesian[2] / norm),
                             cartesian[3]);
        } else {
                vector4_make(dest, 0.0f, 0.0f, 0.0f, cartesian[3]);
        }
}

/**
 * Converts a spherical coordinate vector into a cartesian coordinate
 * vector (standard representation).
 *
 * @param dest destination vector
 * @param spherical source vector
 */
static inline void spherical_to_cartesian (vector4 dest, vector4 spherical)
{
        vector4_make(dest,
                     spherical[0] * sinf (spherical[2]) * cosf (spherical[1]),
                     spherical[0] * sinf (spherical[2]) * sinf (spherical[1]),
                     spherical[0] * cosf (spherical[2]),
                     spherical[3]);
}

/**
 * and matrices, in row order:
 *
 * matrix4 [0x00] ... [0x03]
 *         [0x04] ... [0x07]
 *         [0x08] ... [0x0b]
 *         [0x0c] ... [0x0f]
 */
typedef float matrix4[4*4];

/**
 * The null matrix
 */
static inline void matrix4_null (matrix4 self)
{
        int i;
        for (i = 0; i < 0x10; i++) {
                self[i] = 0.0f;
        }
}

/**
 * The identity matrix
 */
static inline void matrix4_identity (matrix4 self)
{
        matrix4_null (self);
        self[0x00] = 1.0f;
        self[0x05] = 1.0f;
        self[0x0a] = 1.0f;
        self[0x0f] = 1.0f;
}

/**
 * Copying two matrices
 */
static inline void matrix4_copy (matrix4 dst, matrix4 src)
{
        int i;
        for (i = 0; i < 0x10; i++) {
                dst[i] = src[i];
        }
}

/**
 * Multiplying two matrices
 */
static inline void matrix4_mul(matrix4 adst, matrix4 b)
{
        matrix4 result;
        matrix4_null(result);

        for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                        result[4*row + col] =
                                b[4*row + 0] * adst[4*0 + col] +
                                b[4*row + 1] * adst[4*1 + col] +
                                b[4*row + 2] * adst[4*2 + col] +
                                b[4*row + 3] * adst[4*3 + col];
                }
        }

        matrix4_copy(adst, result);
}

/* 3- Quaternions */

/**
 * Makes a quaternion representing the given rotation.
 *
 * @param quaternion as a vector4
 * @param angle of rotation
 * @param axis of rotation
 */
static inline void quaternion_make_rotation (vector4 quaternion, float angle,
                vector4 axis)
{
        vector4 v;
        vector4_copy (v, axis);

        {
                float const vn = vector4_norm3(v);
                if (vn <= 0.0f) {
                        /* error case */
                        vector4_null (quaternion);
                } else {
                        vector4_scale3 (v, vn * sinf(angle / 2.0f));
                        vector4_make (quaternion,
                                      cosf(angle / 2.f), v[0], v[1], v[2]);

                }
        }
}

/**
 * Converts a quaternion into a transform matrix.
 *
 * @param matrix destination matrix
 * @param q quaternion
 * @see quaternion_make_rotation
 */
static inline void matrix4_from_quaternion (matrix4 matrix, vector4 q)
{
        float const norm2 = q[0]*q[0] + q[1]*q[1] + q[2]*q[2] + q[3]*q[3];
        float const s = 2.f / norm2;

        float const q1s = q[1] * s;
        float const q2s = q[2] * s;
        float const q3s = q[3] * s;

        float const q0q1 = q[0] * q1s;
        float const q0q2 = q[0] * q2s;
        float const q0q3 = q[0] * q3s;

        float const q1q1 = q[1] * q1s;
        float const q1q2 = q[1] * q2s;
        float const q1q3 = q[1] * q3s;

        float const q2q2 = q[2] * q2s;
        float const q2q3 = q[2] * q3s;

        float const q3q3 = q[3] * q3s;

        matrix [0 + 0*4] = 1.0f - (q2q2 + q3q3);
        matrix [0 + 1*4] = q1q2 + q0q3;
        matrix [0 + 2*4] = q1q3 - q0q2;
        matrix [0 + 3*4] = 0.0f;

        matrix [1 + 0*4] = q1q2 - q0q3;
        matrix [1 + 1*4] = 1.0f - (q1q1 + q3q3);
        matrix [1 + 2*4] = q2q3 + q0q1;
        matrix [1 + 3*4] = 0.0f;

        matrix [2 + 0*4] = q1q3 + q0q2;
        matrix [2 + 1*4] = q2q3 - q0q1;
        matrix [2 + 2*4] = 1.0f - (q1q1 + q2q2);
        matrix [2 + 3*4] = 0.0f;

        matrix [3 + 0*4] = 0.0f;
        matrix [3 + 1*4] = 0.0f;
        matrix [3 + 2*4] = 0.0f;
        matrix [3 + 3*4] = 1.0f;
}
