//
// Created by valerino on 30/01/19.
//

#pragma once

/**
 * @brief check if bit _n_ is set
 */
#define IS_BIT_SET(_x_, _n_) (((_x_) >> (_n_)) & 1)

/**
 * @brief set Nth bit
 */
#define BIT_SET(_x_, _n_) _x_ |= (1 << _n_)

/**
 * @brief clear Nth bit
 */
#define BIT_CLEAR(_x_, _n_) _x_ &= ~(1 << _n_);

/**
 * @brief toggle (on if off, off if on) Nth bit
 */
#define BIT_TOGGLE(_x_, _n_) _x_ ^= (1 << _n_);
