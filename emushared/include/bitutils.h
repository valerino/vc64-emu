//
// Created by valerino on 30/01/19.
//

#ifndef VC64_EMU_MATHUTILS_H
#define VC64_EMU_MATHUTILS_H

/**
 * check if bit _n_ is set
 */
#define IS_BIT_SET(_x_,_n_) (((_x_)>>(_n_)) & 1)

/**
 * set Nth bit
 */
#define BIT_SET(_x_,_n_) _x_ |= (1 << _n_)

/**
 * clear Nth bit
 */
#define BIT_CLEAR(_x_,_n_) _x_ &= ~(1 << n);

/**
 * toggle (on if off, off if on) Nth bit
 */
#define BIT_TOGGLE(_x_,_n_) _x_ ^= (1 << n);

#endif //VC64_EMU_MATHUTILS_H
