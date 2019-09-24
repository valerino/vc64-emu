//
// Created by valerino on 30/01/19.
//

#pragma once

/**
 * check if bit _n_ is set
 */
#define IS_BIT_SET(_x_, _n_) (((_x_) >> (_n_)) & 1)

/**
 * set Nth bit
 */
#define BIT_SET(_x_, _n_) _x_ |= (1 << _n_)

/**
 * clear Nth bit
 */
#define BIT_CLEAR(_x_, _n_) _x_ &= ~(1 << n);

/**
 * toggle (on if off, off if on) Nth bit
 */
#define BIT_TOGGLE(_x_, _n_) _x_ ^= (1 << n);

/**
 * make an uint32 as ARGB8888
 */
#define MAKEARGB8888(_opacity_, _r_, _g_, _b_)                                 \
    ((_opacity_ << 24) | (_r_ << 16) | (_g_ << 8) | (_b_))
