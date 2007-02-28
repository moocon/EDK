/*++

Copyright (c) 2004 - 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.  


Module Name:

  AutoGen.h
  
Abstract: 

  This file is supposed to be used by a .dxe file. To write common .dxs file for R8.x 
  and R9, a header file named AutoGen.h musted be present. In R8.x-GlueLib code base, 
  this AutoGen.h plays the role as the AutoGen.h in R9. Here the AutoGen.h isn't auto-generated 
  by any tool.

--*/

#ifndef __EDKII_GLUELIB_AUTOGEN_H__
#define __EDKII_GLUELIB_AUTOGEN_H__


//
//  Users can use this macro in .dxs file
//
#ifndef BUILD_WITH_EDKII_GLUE_LIB
  #define BUILD_WITH_EDKII_GLUE_LIB
#endif


#endif