/**************************************************************************\
*//*! \file nanotube_capsule.h
** \author  Neil Turton <neilt@amd.com>
**  \brief  The Nanotube control capsule format.
**   \date  2022-08-15
*//*
\**************************************************************************/

/**************************************************************************
** Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
** SPDX-License-Identifier: MIT
**************************************************************************/

#ifndef NANOTUBE_CAPSULE_H
#define NANOTUBE_CAPSULE_H

///////////////////////////////////////////////////////////////////////////
// This header file describes the format of Nanotube control capsules.
// Each capsule consists of a generic header followed by more detailed
// information.

/*! Multi-byte fields are little-endian */
#define NANOTUBE_CAPSULE_LITTLE_ENDIAN

///////////////////////////////////////////////////////////////////////////
// The format of the generic header.

/*! The request ID is ignored by the device. */
#define NANOTUBE_CAPSULE_REQUEST_ID_OFFSET 0
#define NANOTUBE_CAPSULE_REQUEST_ID_SIZE 2

/*! The resource ID indicates which resource should be accessed. */
#define NANOTUBE_CAPSULE_RESOURCE_ID_OFFSET 2
#define NANOTUBE_CAPSULE_RESOURCE_ID_SIZE 2

/*! The response code indicates whether the request was handled
 *  successfully. */
#define NANOTUBE_CAPSULE_RESPONSE_CODE_OFFSET 4
#define NANOTUBE_CAPSULE_RESPONSE_CODE_SIZE 2

/*! The request was successful. */
#define NANOTUBE_CAPSULE_RESPONSE_CODE_SUCCESS 0

/*! The requester should initialize the response code field to this
 *  value.  If the responder does not handle the request then it will
 *  still have this value.  */
#define NANOTUBE_CAPSULE_RESPONSE_CODE_UNHANDLED 1

/*! The requested resource was not found. */
#define NANOTUBE_CAPSULE_RESPONSE_CODE_UNKNOWN_RESOURCE 2

/*! The requested resource was not found. */
#define NANOTUBE_CAPSULE_RESPONSE_CODE_UNKNOWN_OPCODE 3

/*! The requested entry was not found. */
#define NANOTUBE_CAPSULE_RESPONSE_CODE_NO_ENTRY 4

/*! The size of the generic header. */
#define NANOTUBE_CAPSULE_HEADER_SIZE 6

///////////////////////////////////////////////////////////////////////////
// The format of map requests.

/*! The opcode indicates which map operation should be performed. */
#define NANOTUBE_CAPSULE_MAP_OPCODE_OFFSET (NANOTUBE_CAPSULE_HEADER_SIZE+0)
#define NANOTUBE_CAPSULE_MAP_OPCODE_SIZE 2

/*! Read an entry.  The request contains the key.  The response
 *  contains the value. */
#define NANOTUBE_CAPSULE_MAP_OPCODE_READ 0
/*! Insert an entry.  The request contains the key and value.  The
 *  response is empty. */
#define NANOTUBE_CAPSULE_MAP_OPCODE_INSERT 1
/*! Update an existing entry.  The request contains the key and value.
 *  The response is empty. */
#define NANOTUBE_CAPSULE_MAP_OPCODE_UPDATE 2
/*! Insert or update an entry.  The request contains the key and
 *  value.  The response is empty. */
#define NANOTUBE_CAPSULE_MAP_OPCODE_WRITE 3
/*! Remove an entry.  The request contains the key and value.  The
 *  response is empty. */
#define NANOTUBE_CAPSULE_MAP_OPCODE_REMOVE 4
/*! Find the next key.  The request contains the key.  The response
 *  contains the next key. */
#define NANOTUBE_CAPSULE_MAP_OPCODE_NEXT_KEY 5

/*! The key to access. */
#define NANOTUBE_CAPSULE_MAP_KEY_OFFSET(KEY_SIZE,VALUE_SIZE) \
  (NANOTUBE_CAPSULE_HEADER_SIZE+2)

/*! The associated value. */
#define NANOTUBE_CAPSULE_MAP_VALUE_OFFSET(KEY_SIZE,VALUE_SIZE) \
  (NANOTUBE_CAPSULE_HEADER_SIZE+2+(KEY_SIZE))

/*! The total length of the capsule. */
#define NANOTUBE_CAPSULE_MAP_CAPSULE_SIZE(KEY_SIZE,VALUE_SIZE) \
  (NANOTUBE_CAPSULE_HEADER_SIZE+2+(KEY_SIZE)+(VALUE_SIZE))

///////////////////////////////////////////////////////////////////////////

#endif // NANOTUBE_CAPSULE_H
