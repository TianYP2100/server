/*****************************************************************************

Copyright (c) 1995, 2016, Oracle and/or its affiliates. All Rights Reserved.
Copyright (c) 2017, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/******************************************************************//**
@file include/mach0data.h
Utilities for converting data from the database file
to the machine format.

Created 11/28/1995 Heikki Tuuri
***********************************************************************/

#ifndef mach0data_h
#define mach0data_h

#include "univ.i"

#ifndef UNIV_INNOCHECKSUM

#include "mtr0types.h"

/* The data and all fields are always stored in a database file
in the same format: ascii, big-endian, ... .
All data in the files MUST be accessed using the functions in this
module. */

/*******************************************************//**
The following function is used to store data in one byte. */
UNIV_INLINE
void
mach_write_to_1(
/*============*/
	byte*	b,	/*!< in: pointer to byte where to store */
	ulint	n);	 /*!< in: ulint integer to be stored, >= 0, < 256 */
/** The following function is used to fetch data from one byte.
@param[in]	b	pointer to a byte to read
@return ulint integer, >= 0, < 256 */
UNIV_INLINE
uint8_t
mach_read_from_1(
	const byte*	b)
	MY_ATTRIBUTE((warn_unused_result));
/*******************************************************//**
The following function is used to store data in two consecutive
bytes. We store the most significant byte to the lower address. */
UNIV_INLINE
void
mach_write_to_2(
/*============*/
	byte*	b,	/*!< in: pointer to two bytes where to store */
	ulint	n);	 /*!< in: ulint integer to be stored, >= 0, < 64k */
#endif /* !UNIV_INNOCHECKSUM */
/** The following function is used to fetch data from 2 consecutive
bytes. The most significant byte is at the lowest address.
@param[in]	b	pointer to 2 bytes where to store
@return 2-byte integer, >= 0, < 64k */
UNIV_INLINE
uint16_t
mach_read_from_2(
	const byte*	b)
	MY_ATTRIBUTE((warn_unused_result));

#ifndef UNIV_INNOCHECKSUM
/********************************************************//**
The following function is used to convert a 16-bit data item
to the canonical format, for fast bytewise equality test
against memory.
@return 16-bit integer in canonical format */
UNIV_INLINE
uint16
mach_encode_2(
/*==========*/
	ulint	n)	/*!< in: integer in machine-dependent format */
	MY_ATTRIBUTE((const));
/********************************************************//**
The following function is used to convert a 16-bit data item
from the canonical format, for fast bytewise equality test
against memory.
@return integer in machine-dependent format */
UNIV_INLINE
ulint
mach_decode_2(
/*==========*/
	uint16	n)	/*!< in: 16-bit integer in canonical format */
	MY_ATTRIBUTE((const));
/*******************************************************//**
The following function is used to store data in 3 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_3(
/*============*/
	byte*	b,	/*!< in: pointer to 3 bytes where to store */
	ulint	n);	 /*!< in: ulint integer to be stored */
/** The following function is used to fetch data from 3 consecutive
bytes. The most significant byte is at the lowest address.
@param[in]	b	pointer to 3 bytes to read
@return 32 bit integer */
UNIV_INLINE
uint32_t
mach_read_from_3(
	const byte*	b)
	MY_ATTRIBUTE((warn_unused_result));
/*******************************************************//**
The following function is used to store data in four consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_4(
/*============*/
	byte*	b,	/*!< in: pointer to four bytes where to store */
	ulint	n);	 /*!< in: ulint integer to be stored */
/** The following function is used to fetch data from 4 consecutive
bytes. The most significant byte is at the lowest address.
@param[in]	b	pointer to 4 bytes to read
@return 32 bit integer */
UNIV_INLINE
uint32_t
mach_read_from_4(
	const byte*	b)
	MY_ATTRIBUTE((warn_unused_result));
/*********************************************************//**
Writes a ulint in a compressed form (1..5 bytes).
@return stored size in bytes */
UNIV_INLINE
ulint
mach_write_compressed(
/*==================*/
	byte*	b,	/*!< in: pointer to memory where to store */
	ulint	n);	/*!< in: ulint integer to be stored */
/*********************************************************//**
Returns the size of an ulint when written in the compressed form.
@return compressed size in bytes */
UNIV_INLINE
ulint
mach_get_compressed_size(
/*=====================*/
	ulint	n)	/*!< in: ulint integer to be stored */
	MY_ATTRIBUTE((const));
/** Read a 32-bit integer in a compressed form.
@param[in,out]	b	pointer to memory where to read;
advanced by the number of bytes consumed
@return unsigned value */
UNIV_INLINE
ib_uint32_t
mach_read_next_compressed(
	const byte**	b);
/*******************************************************//**
The following function is used to store data in 6 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_6(
/*============*/
	byte*		b,	/*!< in: pointer to 6 bytes where to store */
	ib_uint64_t	id);	/*!< in: 48-bit integer */
/********************************************************//**
The following function is used to fetch data from 6 consecutive
bytes. The most significant byte is at the lowest address.
@return 48-bit integer */
UNIV_INLINE
ib_uint64_t
mach_read_from_6(
/*=============*/
	const byte*	b)	/*!< in: pointer to 6 bytes */
	MY_ATTRIBUTE((warn_unused_result));
/*******************************************************//**
The following function is used to store data in 7 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_7(
/*============*/
	byte*		b,	/*!< in: pointer to 7 bytes where to store */
	ib_uint64_t	n);	/*!< in: 56-bit integer */
/********************************************************//**
The following function is used to fetch data from 7 consecutive
bytes. The most significant byte is at the lowest address.
@return 56-bit integer */
UNIV_INLINE
ib_uint64_t
mach_read_from_7(
/*=============*/
	const byte*	b)	/*!< in: pointer to 7 bytes */
	MY_ATTRIBUTE((warn_unused_result));
/*******************************************************//**
The following function is used to store data in 8 consecutive
bytes. We store the most significant byte to the lowest address. */
UNIV_INLINE
void
mach_write_to_8(
/*============*/
	void*		b,	/*!< in: pointer to 8 bytes where to store */
	ib_uint64_t	n);	/*!< in: 64-bit integer to be stored */
/********************************************************//**
The following function is used to fetch data from 8 consecutive
bytes. The most significant byte is at the lowest address.
@return 64-bit integer */
UNIV_INLINE
ib_uint64_t
mach_read_from_8(
/*=============*/
	const byte*	b)	/*!< in: pointer to 8 bytes */
	MY_ATTRIBUTE((warn_unused_result));
/*********************************************************//**
Writes a 64-bit integer in a compressed form (5..9 bytes).
@return size in bytes */
UNIV_INLINE
ulint
mach_u64_write_compressed(
/*======================*/
	byte*		b,	/*!< in: pointer to memory where to store */
	ib_uint64_t	n);	/*!< in: 64-bit integer to be stored */
/** Read a 64-bit integer in a compressed form.
@param[in,out]	b	pointer to memory where to read;
advanced by the number of bytes consumed
@return unsigned value */
UNIV_INLINE
ib_uint64_t
mach_u64_read_next_compressed(
	const byte**	b);
/*********************************************************//**
Writes a 64-bit integer in a compressed form (1..11 bytes).
@return size in bytes */
UNIV_INLINE
ulint
mach_u64_write_much_compressed(
/*===========================*/
	byte*		b,	/*!< in: pointer to memory where to store */
	ib_uint64_t	n);	/*!< in: 64-bit integer to be stored */
/*********************************************************//**
Reads a 64-bit integer in a compressed form.
@return the value read */
UNIV_INLINE
ib_uint64_t
mach_u64_read_much_compressed(
/*==========================*/
	const byte*	b)	/*!< in: pointer to memory from where to read */
	MY_ATTRIBUTE((warn_unused_result));
/** Read a 32-bit integer in a compressed form.
@param[in,out]	ptr	pointer to memory where to read;
advanced by the number of bytes consumed, or set NULL if out of space
@param[in]	end_ptr	end of the buffer
@return unsigned value */
ib_uint32_t
mach_parse_compressed(
	const byte**	ptr,
	const byte*	end_ptr);
/** Read a 64-bit integer in a compressed form.
@param[in,out]	ptr	pointer to memory where to read;
advanced by the number of bytes consumed, or set NULL if out of space
@param[in]	end_ptr	end of the buffer
@return unsigned value */
UNIV_INLINE
ib_uint64_t
mach_u64_parse_compressed(
	const byte**	ptr,
	const byte*	end_ptr);

/*********************************************************//**
Reads a double. It is stored in a little-endian format.
@return double read */
UNIV_INLINE
double
mach_double_read(
/*=============*/
	const byte*	b)	/*!< in: pointer to memory from where to read */
	MY_ATTRIBUTE((warn_unused_result));
/*********************************************************//**
Writes a double. It is stored in a little-endian format. */
UNIV_INLINE
void
mach_double_write(
/*==============*/
	byte*	b,	/*!< in: pointer to memory where to write */
	double	d);	/*!< in: double */
/*********************************************************//**
Reads a float. It is stored in a little-endian format.
@return float read */
UNIV_INLINE
float
mach_float_read(
/*============*/
	const byte*	b)	/*!< in: pointer to memory from where to read */
	MY_ATTRIBUTE((warn_unused_result));
/*********************************************************//**
Writes a float. It is stored in a little-endian format. */
UNIV_INLINE
void
mach_float_write(
/*=============*/
	byte*	b,	/*!< in: pointer to memory where to write */
	float	d);	/*!< in: float */
/*********************************************************//**
Reads a ulint stored in the little-endian format.
@return unsigned long int */
UNIV_INLINE
ulint
mach_read_from_n_little_endian(
/*===========================*/
	const byte*	buf,		/*!< in: from where to read */
	ulint		buf_size)	/*!< in: from how many bytes to read */
	MY_ATTRIBUTE((warn_unused_result));


/** Reads a 64 bit stored in big endian format
@param	buf		From where to read
@return uint64_t */
UNIV_INLINE
uint64_t
mach_read_uint64_little_endian(const byte* buf)
{
#ifdef WORDS_BIGENDIAN
  return
    uint64_t(buf[0])       | uint64_t(buf[1]) << 8 |
    uint64_t(buf[2]) << 16 | uint64_t(buf[3]) << 24 |
    uint64_t(buf[4]) << 32 | uint64_t(buf[5]) << 40 |
    uint64_t(buf[6]) << 48 | uint64_t(buf[7]) << 56;
#else
  uint64_t n;
  memcpy(&n, buf, sizeof(uint64_t));
  return n;
#endif
}

/*********************************************************//**
Writes a ulint in the little-endian format. */
UNIV_INLINE
void
mach_write_to_n_little_endian(
/*==========================*/
	byte*	dest,		/*!< in: where to write */
	ulint	dest_size,	/*!< in: into how many bytes to write */
	ulint	n);		/*!< in: unsigned long int to write */
/*********************************************************//**
Reads a ulint stored in the little-endian format.
@return unsigned long int */
UNIV_INLINE
ulint
mach_read_from_2_little_endian(
/*===========================*/
	const byte*	buf)		/*!< in: from where to read */
	MY_ATTRIBUTE((warn_unused_result));
/*********************************************************//**
Writes a ulint in the little-endian format. */
UNIV_INLINE
void
mach_write_to_2_little_endian(
/*==========================*/
	byte*	dest,		/*!< in: where to write */
	ulint	n);		/*!< in: unsigned long int to write */
/*********************************************************//**
Convert integral type from storage byte order (big endian) to
host byte order.
@return integer value */
UNIV_INLINE
ib_uint64_t
mach_read_int_type(
/*===============*/
	const byte*	src,		/*!< in: where to read from */
	ulint		len,		/*!< in: length of src */
	ibool		unsigned_type);	/*!< in: signed or unsigned flag */

/*************************************************************
Convert a ulonglong integer from host byte order to (big-endian)
storage byte order. */
UNIV_INLINE
void
mach_write_ulonglong(
/*=================*/
	byte*		dest,		/*!< in: where to write */
	ulonglong	src,		/*!< in: where to read from */
	ulint		len,		/*!< in: length of dest */
	bool		usign);		/*!< in: signed or unsigned flag */

#endif /* !UNIV_INNOCHECKSUM */

/** Read 1 to 4 bytes from a file page buffered in the buffer pool.
@param[in]	ptr	pointer where to read
@param[in]	type	MLOG_1BYTE, MLOG_2BYTES, or MLOG_4BYTES
@return value read */
UNIV_INLINE
ulint
mach_read_ulint(
	const byte*	ptr,
	mlog_id_t	type)
	MY_ATTRIBUTE((warn_unused_result));

#include "mach0data.inl"

#endif
