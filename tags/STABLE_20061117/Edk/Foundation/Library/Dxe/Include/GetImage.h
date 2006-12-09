/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  GetImage.h

Abstract:

  Image data retrieval support for common use.

--*/

#ifndef _GET_IMAGE_H_
#define _GET_IMAGE_H_
#include "EfiImageFormat.h"

EFI_STATUS
GetImage (
  IN  EFI_GUID           *NameGuid,
  IN  EFI_SECTION_TYPE   SectionType,
  OUT VOID               **Buffer,
  OUT UINTN              *Size
  )
/*++

Routine Description:
  Enumerate all the FVs, and fill Buffer with the SectionType section content in NameGuid file.

  Note:
  1. when SectionType is EFI_SECTION_PE32, it tries to read NameGuid file 
      after failure on read required section.
  2. Callee allocates memory, which caller is responsible to free.

Arguments:

  NameGuid            - Pointer to EFI_GUID, which is file name.
  SectionType         - Required section type.
  Buffer              - Pointer to a pointer in which the read content is returned.
                          Caller is responsible to free Buffer.
  Size                - Pointer to a UINTN, which indicates the size of returned *Buffer.

Returns:
  EFI_NOT_FOUND       - Required content can not be found.
  EFI_SUCCESS         - Required content can be found, but whether the Buffer is filled with section content or not
                          depends on the Buffer and Size.
--*/
;

#endif //_GET_IMAGE_H_