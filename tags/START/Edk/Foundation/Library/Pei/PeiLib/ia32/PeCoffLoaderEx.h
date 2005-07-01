/*++

Copyright 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    PeCoffLoaderEx.h

Abstract:

    IA-32 Specific relocation fixups

Revision History

--*/

//
// Define macro to determine if the machine type is supported.
// Returns 0 if the machine is not supported, Not 0 otherwise.
//
#define EFI_IMAGE_MACHINE_TYPE_SUPPORTED(Machine) \
  ((Machine) == EFI_IMAGE_MACHINE_IA32 || \
   (Machine) == EFI_IMAGE_MACHINE_EBC)

EFI_STATUS
PeCoffLoaderRelocateImageEx (
    IN UINT16      *Reloc,
    IN OUT CHAR8   *Fixup, 
    IN OUT CHAR8   **FixupData,
    IN UINT64      Adjust
    );