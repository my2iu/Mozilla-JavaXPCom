/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Java XPCOM Bindings.
 *
 * The Initial Developer of the Original Code is IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * IBM Corporation. All Rights Reserved.
 *
 * Contributor(s):
 *   Javier Pedemonte (jhpedemonte@gmail.com)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

 #include "mozilla/Char16.h"   // Force include this early to prevent build problems with Windows
#include "nsEmbedString.h"

#include "nsJavaInterfaces.h"
#include "nsJavaWrapper.h"
#include "nsJavaXPTCStub.h"
#include "nsJavaXPCOMBindingUtils.h"
#include "jni.h"
#include "xptcall.h"
#include "nsIInterfaceInfoManager.h"
#include "nsStringAPI.h"
#include "nsCRT.h"
#include "prmem.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"

static nsID nullID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};


nsresult
CreateJavaArray(JNIEnv* env, PRUint8 aType, PRUint32 aSize, const nsID& aIID,
                jobject* aResult)
{
  jobject array = nullptr;
  switch (aType)
  {
    case nsXPTType::T_I8:
      array = env->NewByteArray(aSize);
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:
      array = env->NewShortArray(aSize);
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:
      array = env->NewIntArray(aSize);
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:
      array = env->NewLongArray(aSize);
      break;

    case nsXPTType::T_FLOAT:
      array = env->NewFloatArray(aSize);
      break;

    // XXX how do we handle unsigned 64-bit values?
    case nsXPTType::T_U64:
    case nsXPTType::T_DOUBLE:
      array = env->NewDoubleArray(aSize);
      break;

    case nsXPTType::T_BOOL:
      array = env->NewBooleanArray(aSize);
      break;

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
      array = env->NewCharArray(aSize);
      break;

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    case nsXPTType::T_IID:
    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      array = env->NewObjectArray(aSize, stringClass, nullptr);
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      nsCOMPtr<nsIInterfaceInfoManager>
        iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
      NS_ASSERTION(iim, "Failed to get InterfaceInfoManager");
      if (!iim)
        return NS_ERROR_FAILURE;

      // Get interface info for given IID
      nsCOMPtr<nsIInterfaceInfo> info;
      nsresult rv = iim->GetInfoForIID(&aIID, getter_AddRefs(info));
      if (NS_FAILED(rv))
        return rv;

      // Get interface name
      const char* iface_name;
      rv = info->GetNameShared(&iface_name);
      if (NS_FAILED(rv))
        return rv;

      // Create proper Java interface name
      nsEmbedCString class_name("org/mozilla/interfaces/");
      class_name.AppendASCII(iface_name);
      jclass ifaceClass = env->FindClass(class_name.get());
      if (!ifaceClass)
        return NS_ERROR_FAILURE;

      array = env->NewObjectArray(aSize, ifaceClass, nullptr);
      break;
    }

    case nsXPTType::T_VOID:
      array = env->NewLongArray(aSize);
      break;

    default:
      NS_WARNING("unknown type");
      return NS_ERROR_FAILURE;
  }

  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = array;
  return NS_OK;
}

nsresult
GetNativeArrayElement(PRUint8 aType, void* aArray, PRUint32 aIndex,
                      nsXPTCVariant* aResult)
{
  switch (aType)
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      aResult->val.u8 = static_cast<PRUint8*>(aArray)[aIndex];
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      aResult->val.u16 = static_cast<PRUint16*>(aArray)[aIndex];
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      aResult->val.u32 = static_cast<PRUint32*>(aArray)[aIndex];
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      aResult->val.u64 = static_cast<PRUint64*>(aArray)[aIndex];
      break;

    case nsXPTType::T_FLOAT:
      aResult->val.f = static_cast<float*>(aArray)[aIndex];
      break;

    case nsXPTType::T_DOUBLE:
      aResult->val.d = static_cast<double*>(aArray)[aIndex];
      break;

    case nsXPTType::T_BOOL:
      aResult->val.b = static_cast<PRBool*>(aArray)[aIndex];
      break;

    case nsXPTType::T_CHAR:
      aResult->val.c = static_cast<char*>(aArray)[aIndex];
      break;

    case nsXPTType::T_WCHAR:
      aResult->val.wc = static_cast<PRUnichar*>(aArray)[aIndex];
      break;

    case nsXPTType::T_CHAR_STR:
      aResult->val.p = static_cast<char**>(aArray)[aIndex];
      break;

    case nsXPTType::T_WCHAR_STR:
      aResult->val.p = static_cast<PRUnichar**>(aArray)[aIndex];
      break;

    case nsXPTType::T_IID:
      aResult->val.p = static_cast<nsID**>(aArray)[aIndex];
      break;

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
      aResult->val.p = static_cast<nsISupports**>(aArray)[aIndex];
      break;

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
      aResult->val.p = static_cast<nsString**>(aArray)[aIndex];
      break;

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
      aResult->val.p = static_cast<nsCString**>(aArray)[aIndex];
      break;

    case nsXPTType::T_VOID:
      aResult->val.p = static_cast<void**>(aArray)[aIndex];
      break;

    default:
      NS_WARNING("unknown type");
      return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
CreateNativeArray(PRUint8 aType, PRUint32 aSize, void** aResult)
{
  void* array = nullptr;
  switch (aType)
  {
    case nsXPTType::T_I8:
    case nsXPTType::T_U8:
      array = PR_Malloc(aSize * sizeof(PRUint8));
      break;

    case nsXPTType::T_I16:
    case nsXPTType::T_U16:
      array = PR_Malloc(aSize * sizeof(PRUint16));
      break;

    case nsXPTType::T_I32:
    case nsXPTType::T_U32:
      array = PR_Malloc(aSize * sizeof(PRUint32));
      break;

    case nsXPTType::T_I64:
    case nsXPTType::T_U64:
      array = PR_Malloc(aSize * sizeof(PRUint64));
      break;

    case nsXPTType::T_FLOAT:
      array = PR_Malloc(aSize * sizeof(float));
      break;

    case nsXPTType::T_DOUBLE:
      array = PR_Malloc(aSize * sizeof(double));
      break;

    case nsXPTType::T_BOOL:
      array = PR_Malloc(aSize * sizeof(PRBool));
      break;

    case nsXPTType::T_CHAR:
      array = PR_Malloc(aSize * sizeof(char));
      break;

    case nsXPTType::T_WCHAR:
      array = PR_Malloc(aSize * sizeof(PRUnichar));
      break;

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    case nsXPTType::T_IID:
    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
      array = PR_Malloc(aSize * sizeof(void*));
      break;

    case nsXPTType::T_VOID:
      array = PR_Malloc(aSize * sizeof(void*));
      break;

    default:
      NS_WARNING("unknown type");
      return NS_ERROR_FAILURE;
  }

  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = array;
  return NS_OK;
}

// TODO: Is this the correct way to emulate the old behaviour? 
// TODO: I should probably be setting the aVariant.type all the time maybe?
void setValIsInterface(nsXPTCVariant &aVariant)
{
  // TODO: This ignore the possibility of T_INTERFACE_IS
  aVariant.type = nsXPTType::T_INTERFACE;
  aVariant.SetValNeedsCleanup();
}

void setValIsDOMString(nsXPTCVariant &aVariant)
{
  aVariant.type = nsXPTType::T_DOMSTRING;
  aVariant.SetValNeedsCleanup();
}

void setValIsCString(nsXPTCVariant &aVariant)
{
  aVariant.type = nsXPTType::T_CSTRING;
  aVariant.SetValNeedsCleanup();
}

void setValIsUTF8String(nsXPTCVariant &aVariant)
{
  aVariant.type = nsXPTType::T_UTF8STRING;
  aVariant.SetValNeedsCleanup();
}

/**
 * Handle 'in' and 'inout' params.
 */
nsresult
SetupParams(JNIEnv *env, const jobject aParam, PRUint8 aType, PRBool aIsOut,
            const nsID& aIID, PRUint8 aArrayType, PRUint32 aArraySize,
            PRBool aIsArrayElement, PRUint32 aIndex, nsXPTCVariant &aVariant)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case nsXPTType::T_I8:
    {
      LOG(("byte\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        aVariant.val.i8 = env->CallByteMethod(aParam, byteValueMID);
      } else { // 'inout' & 'array'
        jbyte value;
        if (aParam) {
          env->GetByteArrayRegion((jbyteArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            aVariant.val.i8 = value;
			aVariant.SetIndirect();
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          static_cast<PRInt8*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:   // C++ unsigned octet <=> Java short
    {
      LOG(("short\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        jshort value = env->CallShortMethod(aParam, shortValueMID);
        if (aType == nsXPTType::T_I16)
          aVariant.val.i16 = value;
        else
          aVariant.val.u8 = value;
      } else { // 'inout' & 'array'
        jshort value;
        if (aParam) {
          env->GetShortArrayRegion((jshortArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            if (aType == nsXPTType::T_I16)
              aVariant.val.i16 = value;
            else
              aVariant.val.u8 = value;
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          if (aType == nsXPTType::T_I16)
            static_cast<PRInt16*>(aVariant.val.p)[aIndex] = value;
          else
            static_cast<PRUint8*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:  // C++ unsigned short <=> Java int
    {
      LOG(("int\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        jint value = env->CallIntMethod(aParam, intValueMID);
        if (aType == nsXPTType::T_I32)
          aVariant.val.i32 = value;
        else
          aVariant.val.u16 = value;
      } else { // 'inout' & 'array'
        jint value;
        if (aParam) {
          env->GetIntArrayRegion((jintArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            if (aType == nsXPTType::T_I32)
              aVariant.val.i32 = value;
            else
              aVariant.val.u16 = value;
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          if (aType == nsXPTType::T_I32)
            static_cast<PRInt32*>(aVariant.val.p)[aIndex] = value;
          else
            static_cast<PRUint16*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:  // C++ unsigned int <=> Java long
    {
      LOG(("long\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        jlong value = env->CallLongMethod(aParam, longValueMID);
        if (aType == nsXPTType::T_I64)
          aVariant.val.i64 = value;
        else
          aVariant.val.u32 = value;
      } else { // 'inout' & 'array'
        jlong value;
        if (aParam) {
          env->GetLongArrayRegion((jlongArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            if (aType == nsXPTType::T_I64)
              aVariant.val.i64 = value;
            else
              aVariant.val.u32 = value;
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          if (aType == nsXPTType::T_I64)
            static_cast<PRInt64*>(aVariant.val.p)[aIndex] = value;
          else
            static_cast<PRUint32*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_FLOAT:
    {
      LOG(("float\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        aVariant.val.f = env->CallFloatMethod(aParam, floatValueMID);
      } else { // 'inout' & 'array'
        jfloat value;
        if (aParam) {
          env->GetFloatArrayRegion((jfloatArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            aVariant.val.f = value;
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          static_cast<float*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    // XXX how do we handle unsigned 64-bit value?
    case nsXPTType::T_U64:  // C++ unsigned long <=> Java double
    case nsXPTType::T_DOUBLE:
    {
      LOG(("double\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        jdouble value = env->CallDoubleMethod(aParam, doubleValueMID);
        if (aType == nsXPTType::T_DOUBLE)
          aVariant.val.d = value;
        else
          aVariant.val.u64 = static_cast<PRUint64>(value);
      } else { // 'inout' & 'array'
        jdouble value;
        if (aParam) {
          env->GetDoubleArrayRegion((jdoubleArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            if (aType == nsXPTType::T_DOUBLE)
              aVariant.val.d = value;
            else
              aVariant.val.u64 = static_cast<PRUint64>(value);
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          if (aType == nsXPTType::T_DOUBLE)
            static_cast<double*>(aVariant.val.p)[aIndex] = value;
          else
            static_cast<PRUint64*>(aVariant.val.p)[aIndex] =
                                                static_cast<PRUint64>(value);
        }
      }
      break;
    }

    case nsXPTType::T_BOOL:
    {
      LOG(("boolean\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        aVariant.val.b = env->CallBooleanMethod(aParam, booleanValueMID);
      } else { // 'inout' & 'array'
        jboolean value;
        if (aParam) {
          env->GetBooleanArrayRegion((jbooleanArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            aVariant.val.b = value;
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          static_cast<PRBool*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_CHAR:
    {
      LOG(("char\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        aVariant.val.c = env->CallCharMethod(aParam, charValueMID);
      } else { // 'inout' & 'array'
        jchar value;
        if (aParam) {
          env->GetCharArrayRegion((jcharArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            aVariant.val.c = value;
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          static_cast<char*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_WCHAR:
    {
      LOG(("char\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        aVariant.val.wc = env->CallCharMethod(aParam, charValueMID);
      } else { // 'inout' & 'array'
        jchar value;
        if (aParam) {
          env->GetCharArrayRegion((jcharArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            aVariant.val.wc = value;
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          static_cast<PRUnichar*>(aVariant.val.p)[aIndex] = value;
        }
      }
      break;
    }

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    {
      LOG(("String\n"));
      jstring data = nullptr;
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        data = (jstring) aParam;
      } else if (aParam) {  // 'inout' & 'array'
        data = (jstring) env->GetObjectArrayElement((jobjectArray) aParam,
                                                    aIndex);
      }

      void* buf = nullptr;
      if (data) {
        jsize uniLength = env->GetStringLength(data);
        if (uniLength > 0) {
          if (aType == nsXPTType::T_CHAR_STR) {
            jsize utf8Length = env->GetStringUTFLength(data);
            buf = moz_xmalloc((utf8Length + 1) * sizeof(char));
            if (!buf) {
              rv = NS_ERROR_OUT_OF_MEMORY;
              break;
            }

            char* char_str = static_cast<char*>(buf);
            env->GetStringUTFRegion(data, 0, uniLength, char_str);
            char_str[utf8Length] = '\0';

          } else {  // if T_WCHAR_STR
            buf = moz_xmalloc((uniLength + 1) * sizeof(jchar));
            if (!buf) {
              rv = NS_ERROR_OUT_OF_MEMORY;
              break;
            }

            jchar* jchar_str = static_cast<jchar*>(buf);
            env->GetStringRegion(data, 0, uniLength, jchar_str);
            jchar_str[uniLength] = '\0';
          }
        } else {
          // create empty string
          buf = moz_xmalloc(2);
          if (!buf) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
          ((jchar*)buf)[0] = '\0';
        }
      }

      if (!aIsArrayElement) { // 'in' & 'inout'
        aVariant.val.p = buf;
        if (aIsOut) { // 'inout'
		  aVariant.SetIndirect();
          //aVariant.ptr = &aVariant.val;
          //aVariant.SetPtrIsData();
        }
      } else {  // 'array'
        if (aType == nsXPTType::T_CHAR_STR) {
          char* str = static_cast<char*>(buf);
          static_cast<char**>(aVariant.val.p)[aIndex] = str;
        } else {
          PRUnichar* str = static_cast<PRUnichar*>(buf);
          static_cast<PRUnichar**>(aVariant.val.p)[aIndex] = str;
        }
      }
      break;
    }

    case nsXPTType::T_IID:
    {
      LOG(("String(IID)\n"));
      jstring data = nullptr;
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        data = (jstring) aParam;
      } else if (aParam) {  // 'inout' & 'array'
        data = (jstring) env->GetObjectArrayElement((jobjectArray) aParam,
                                                    aIndex);
      }

      nsID* iid = new nsID;
      if (!iid) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }
      if (data) {
        // extract IID string from Java string
        const char* str = env->GetStringUTFChars(data, nullptr);
        if (!str) {
          rv = NS_ERROR_OUT_OF_MEMORY;
          break;
        }

        // parse string into IID object
        iid->Parse(str);
        env->ReleaseStringUTFChars(data, str);
      } else {
        *iid = nullID;
      }

      if (!aIsArrayElement) { // 'in' & 'inout'
        aVariant.val.p = iid;
        if (aIsOut) { // 'inout'
		  aVariant.SetIndirect();
          //aVariant.ptr = &aVariant.val;
          //aVariant.SetPtrIsData();
        }
      } else {  // 'array'
        static_cast<nsID**>(aVariant.val.p)[aIndex] = iid;
      }
      break;
    }

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      LOG(("nsISupports\n"));
      jobject java_obj = nullptr;
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        java_obj = (jobject) aParam;
      } else if (aParam) {  // 'inout' & 'array'
        java_obj = (jobject) env->GetObjectArrayElement((jobjectArray) aParam,
                                                        aIndex);
      }

      void* xpcom_obj;
      if (java_obj) {
        // If the requested interface is nsIWeakReference, then we look for or
        // create a stub for the nsISupports interface.  Then we create a weak
        // reference from that stub.
        PRBool isWeakRef;
        nsID iid;
        if (aIID.Equals(NS_GET_IID(nsIWeakReference))) {
          isWeakRef = PR_TRUE;
          iid = NS_GET_IID(nsISupports);
        } else {
          isWeakRef = PR_FALSE;
          iid = aIID;
        }

        rv = JavaObjectToNativeInterface(env, java_obj, iid, &xpcom_obj);
        if (NS_FAILED(rv))
          break;
        nsISupports* pre_xpcom_obj = (nsISupports*)xpcom_obj;
        rv = pre_xpcom_obj->QueryInterface(iid, &xpcom_obj);
        if (NS_FAILED(rv))
          break;
        // TODO: Is it right to release here after an additional reference was made through QueryInterface?
        // TODO: Why wasn't this line here before unless this is wrong?
        NS_RELEASE(pre_xpcom_obj);

        // If the function expects a weak reference, then we need to
        // create it here.
        if (isWeakRef) {
          nsISupports* isupports = (nsISupports*) xpcom_obj;
          nsCOMPtr<nsISupportsWeakReference> supportsweak =
                  do_QueryInterface(isupports);
          if (supportsweak) {
            nsWeakPtr weakref;
            supportsweak->GetWeakReference(getter_AddRefs(weakref));
            NS_RELEASE(isupports);
            xpcom_obj = weakref;
            NS_ADDREF((nsISupports*) xpcom_obj);
          } else {
            xpcom_obj = nullptr;
          }
        }
      } else {
        xpcom_obj = nullptr;
      }

      if (!aIsArrayElement) { // 'in' & 'inout'
        aVariant.val.p = xpcom_obj;
		setValIsInterface(aVariant);
        //aVariant.SetValIsInterface();
        if (aIsOut) { // 'inout'
		  aVariant.SetIndirect();
          //aVariant.ptr = &aVariant.val;
          //aVariant.SetPtrIsData();
        }
      } else {  // 'array'
        static_cast<void**>(aVariant.val.p)[aIndex] = xpcom_obj;
      }
      break;
    }

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      LOG(("String\n"));
      // Expecting only 'in' and 'in dipper'
      NS_PRECONDITION(!aIsOut, "unexpected param descriptor");
      if (aIsOut) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      jstring jstr = static_cast<jstring>(aParam);
      nsString* str = jstring_to_nsString(env, jstr);
      if (!str) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      aVariant.val.p = str;
      setValIsDOMString(aVariant);
      //aVariant.SetValIsDOMString();
      break;
    }

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      LOG(("StringUTF\n"));
      // Expecting only 'in' and 'in dipper'
      NS_PRECONDITION(!aIsOut, "unexpected param descriptor");
      if (aIsOut) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      jstring jstr = static_cast<jstring>(aParam);
      nsCString* str = jstring_to_nsCString(env, jstr);
      if (!str) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      aVariant.val.p = str;
      if (aType == nsXPTType::T_CSTRING) {
        setValIsCString(aVariant);
		//aVariant.SetValIsCString();
      } else {
        setValIsUTF8String(aVariant);
		//aVariant.SetValIsUTF8String();
      }
      break;
    }

    // handle "void *" as an "long" in Java
    case nsXPTType::T_VOID:
    {
      LOG(("long (void*)\n"));
      if (!aIsOut && !aIsArrayElement) {  // 'in'
        aVariant.val.p =
          reinterpret_cast<void*>(env->CallLongMethod(aParam, longValueMID));
      } else { // 'inout' & 'array'
        jlong value;
        if (aParam) {
          env->GetLongArrayRegion((jlongArray) aParam, aIndex, 1, &value);
        }

        if (aIsOut) { // 'inout'
          if (aParam) {
            aVariant.val.p = reinterpret_cast<void*>(value);
			aVariant.SetIndirect();
            //aVariant.ptr = &aVariant.val;
          } else {
            aVariant.ptr = nullptr;
          }
          //aVariant.SetPtrIsData();
        } else {  // 'array'
          static_cast<void**>(aVariant.val.p)[aIndex] =
                  reinterpret_cast<void*>(value);
        }
      }
      break;
    }

    case nsXPTType::T_ARRAY:
    {
      jobject sourceArray = nullptr;
      if (!aIsOut) {  // 'in'
        sourceArray = aParam;
      } else if (aParam) {  // 'inout'
        jobjectArray array = static_cast<jobjectArray>(aParam);
        sourceArray = env->GetObjectArrayElement(array, 0);
      }

      if (sourceArray) {
        rv = CreateNativeArray(aArrayType, aArraySize, &aVariant.val.p);

        for (PRUint32 i = 0; i < aArraySize && NS_SUCCEEDED(rv); i++) {
          rv = SetupParams(env, sourceArray, aArrayType, PR_FALSE, aIID, 0, 0,
                           PR_TRUE, i, aVariant);
        }
      }

      if (aIsOut) { // 'inout'
	    aVariant.SetIndirect();
        //aVariant.ptr = &aVariant.val.p;
        //aVariant.SetPtrIsData();
      }
      break;
    }

    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    {
      NS_PRECONDITION(!aIsArrayElement, "sized string array not supported");
      
      LOG(("Sized string\n"));
      jstring data = nullptr;
      if (!aIsOut) {  // 'in'
        data = (jstring) aParam;
      } else if (aParam) {  // 'inout'
        data = (jstring) env->GetObjectArrayElement((jobjectArray) aParam,
                                                    aIndex);
      }

      PRUint32 length = 0;
      if (data) {
        if (aType == nsXPTType::T_PSTRING_SIZE_IS) {
          length = env->GetStringUTFLength(data);
        } else {
          length = env->GetStringLength(data);
        }
        if (length > aArraySize) {
          rv = NS_ERROR_ILLEGAL_VALUE;
          break;
        }
      }

      PRUint32 size_of_char = (aType == nsXPTType::T_PSTRING_SIZE_IS) ?
                              sizeof(char) : sizeof(jchar);
      PRUint32 allocLength = (aArraySize + 1) * size_of_char;
      void* buf = moz_xmalloc(allocLength);
      if (!buf) {
        rv = NS_ERROR_OUT_OF_MEMORY;
        break;
      }

      if (data) {
        if (aType == nsXPTType::T_PSTRING_SIZE_IS) {
          const char* str = env->GetStringUTFChars(data, nullptr);
          if (!str) {
            moz_free(buf);
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
          memcpy(buf, str, length);
          env->ReleaseStringUTFChars(data, str);
        } else {
          jchar* jchar_str = static_cast<jchar*>(buf);
          env->GetStringRegion(data, 0, length, jchar_str);
        }
      }
      
      aVariant.val.p = buf;
      if (aIsOut) { // 'inout'
	    aVariant.SetIndirect();
        //aVariant.ptr = &aVariant.val;
        //aVariant.SetPtrIsData();
      }

      break;
    }

    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  return rv;
}

/**
 * Does any cleanup from objects created in SetupParams, as well as converting
 * any out params to Java.
 *
 * NOTE: If aInvokeResult is an error condition, then we just do cleanup in
 *  this function.
 */
nsresult
FinalizeParams(JNIEnv *env, const nsXPTParamInfo &aParamInfo, PRUint8 aType,
               nsXPTCVariant &aVariant, const nsID& aIID,
               PRBool aIsArrayElement, PRUint8 aArrayType, PRUint32 aArraySize,
               PRUint32 aIndex, nsresult aInvokeResult, jobject* aParam)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case nsXPTType::T_I8:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jbyte value = aVariant.val.i8;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(byteClass, byteInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetByteArrayRegion((jbyteArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_I16:
    case nsXPTType::T_U8:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jshort value = (aType == nsXPTType::T_I16) ? aVariant.val.i16 :
                                                     aVariant.val.u8;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(shortClass, shortInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && aParam) {
          env->SetShortArrayRegion((jshortArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_I32:
    case nsXPTType::T_U16:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jint value = (aType == nsXPTType::T_I32) ? aVariant.val.i32 :
                                                   aVariant.val.u16;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(intClass, intInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetIntArrayRegion((jintArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_I64:
    case nsXPTType::T_U32:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jlong value = (aType == nsXPTType::T_I64) ? aVariant.val.i64 :
                                                    aVariant.val.u32;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(longClass, longInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetLongArrayRegion((jlongArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_FLOAT:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jfloat value = aVariant.val.f;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(floatClass, floatInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetFloatArrayRegion((jfloatArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    // XXX how do we handle unsigned 64-bit values?
    case nsXPTType::T_U64:
    case nsXPTType::T_DOUBLE:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jdouble value = (aType == nsXPTType::T_DOUBLE) ? aVariant.val.d :
                                                         aVariant.val.u64;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(doubleClass, doubleInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetDoubleArrayRegion((jdoubleArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_BOOL:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jboolean value = aVariant.val.b;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(booleanClass, booleanInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetBooleanArrayRegion((jbooleanArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_CHAR:
    case nsXPTType::T_WCHAR:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jchar value;
        if (aType == nsXPTType::T_CHAR)
          value = aVariant.val.c;
        else
          value = aVariant.val.wc;
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(charClass, charInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetCharArrayRegion((jcharArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_CHAR_STR:
    case nsXPTType::T_WCHAR_STR:
    {
      if ((aParamInfo.IsOut() || aIsArrayElement) &&
          NS_SUCCEEDED(aInvokeResult))
      {
        // create new string from data
        jstring str = nullptr;
        if (aVariant.val.p) {
          if (aType == nsXPTType::T_CHAR_STR) {
            str = env->NewStringUTF((const char*) aVariant.val.p);
          } else {
            PRUint32 length = NS_strlen((const PRUnichar*) aVariant.val.p);
            str = env->NewString((const jchar*) aVariant.val.p, length);
          }
          if (!str) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = str;
        } else if (*aParam) {
          // put new string into output array
          env->SetObjectArrayElement((jobjectArray) *aParam, aIndex, str);
        }
      }

      // cleanup
      if (aVariant.val.p)
        moz_free(aVariant.val.p);
      break;
    }

    case nsXPTType::T_IID:
    {
      nsID* iid = static_cast<nsID*>(aVariant.val.p);

      if ((aParamInfo.IsOut() || aIsArrayElement) &&
          NS_SUCCEEDED(aInvokeResult))
      {
        // Create the string from nsID
        jstring str = nullptr;
        if (iid) {
          char iid_str[NSID_LENGTH];
          iid->ToProvidedString(iid_str);
          str = env->NewStringUTF(iid_str);
          if (!str) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = str;
        } else if (*aParam) {
          // put new string into output array
          env->SetObjectArrayElement((jobjectArray) *aParam, aIndex, str);
        }
      }

      // Ordinarily, we would delete 'iid' here.  But we cannot do that until
      // we've handled all of the params.  See comment in CallXPCOMMethod.
      // We can safely delete array elements, though.
      if (aIsArrayElement)
        delete iid;

      break;
    }

    case nsXPTType::T_INTERFACE:
    case nsXPTType::T_INTERFACE_IS:
    {
      nsISupports* xpcom_obj = static_cast<nsISupports*>(aVariant.val.p);

      if ((aParamInfo.IsOut() || aIsArrayElement) &&
          NS_SUCCEEDED(aInvokeResult))
      {
        jobject java_obj = nullptr;
        if (xpcom_obj) {
          // Get matching Java object for given xpcom object
          rv = NativeInterfaceToJavaObject(env, xpcom_obj, aIID, nullptr,
                                           &java_obj);
          if (NS_FAILED(rv))
            break;
        }

        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = java_obj;
        } else if (*aParam) {
          // put new Java object into output array
          env->SetObjectArrayElement((jobjectArray) *aParam, aIndex, java_obj);
        }
      }

      // cleanup
      NS_IF_RELEASE(xpcom_obj);
      break;
    }

    case nsXPTType::T_ASTRING:
    case nsXPTType::T_DOMSTRING:
    {
      NS_PRECONDITION(aParamInfo.IsIn(), "unexpected param descriptor");
      if (!aParamInfo.IsIn()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      nsString* str = static_cast<nsString*>(aVariant.val.p);
      if (NS_SUCCEEDED(aInvokeResult) && aParamInfo.IsDipper()) {
        // Create Java string from returned nsString
        jstring jstr = nullptr;
        if (str && !str->IsVoid()) {
          jstr = env->NewString((const jchar*) str->get(), str->Length());
          if (!jstr) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        *aParam = jstr;        
      }

      // cleanup
      if (str) {
        delete str;
      }
      break;
    }

    case nsXPTType::T_UTF8STRING:
    case nsXPTType::T_CSTRING:
    {
      NS_PRECONDITION(aParamInfo.IsIn(), "unexpected param descriptor");
      if (!aParamInfo.IsIn()) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      nsCString* str = static_cast<nsCString*>(aVariant.val.p);
      if (NS_SUCCEEDED(aInvokeResult) && aParamInfo.IsDipper()) {
        // Create Java string from returned nsString
        jstring jstr = nullptr;
        if (str && !str->IsVoid()) {
          jstr = env->NewStringUTF((const char*) str->get());
          if (!jstr) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        *aParam = jstr;        
      }

      // cleanup
      if (str) {
        delete str;
      }
      break;
    }

    case nsXPTType::T_VOID:
    {
      if (NS_SUCCEEDED(aInvokeResult)) {
        jlong value = reinterpret_cast<jlong>(aVariant.val.p);
        if (aParamInfo.IsRetval() && !aIsArrayElement) {
          *aParam = env->NewObject(longClass, longInitMID, value);
        } else if ((aParamInfo.IsOut() || aIsArrayElement) && *aParam) {
          env->SetLongArrayRegion((jlongArray) *aParam, aIndex, 1, &value);
        }
      }
      break;
    }

    case nsXPTType::T_ARRAY:
    {
      if (aParamInfo.IsOut() && NS_SUCCEEDED(aInvokeResult)) {
        // Create Java array from returned native array
        jobject jarray = nullptr;
        if (aVariant.val.p) {
          rv = CreateJavaArray(env, aArrayType, aArraySize, aIID, &jarray);
          if (NS_FAILED(rv))
            break;

          nsXPTCVariant var;
          for (PRUint32 i = 0; i < aArraySize && NS_SUCCEEDED(rv); i++) {
            rv = GetNativeArrayElement(aArrayType, aVariant.val.p, i, &var);
            if (NS_SUCCEEDED(rv)) {
              rv = FinalizeParams(env, aParamInfo, aArrayType, var, aIID,
                                  PR_TRUE, 0, 0, i, aInvokeResult, &jarray);
            }
          }
        }

        if (aParamInfo.IsRetval()) {
          *aParam = jarray;
        } else if (*aParam) {
          // put new Java array into output array
          env->SetObjectArrayElement((jobjectArray) *aParam, 0, jarray);
        }
      }

      // cleanup
      // If this is not an out param or if the invokeResult is a failure case,
      // then the array elements have not been cleaned up.  Do so now.
      if (!aParamInfo.IsOut() || (NS_FAILED(aInvokeResult) && aVariant.val.p)) {
        nsXPTCVariant var;
        for (PRUint32 i = 0; i < aArraySize; i++) {
          rv = GetNativeArrayElement(aArrayType, aVariant.val.p, i, &var);
          if (NS_SUCCEEDED(rv)) {
            FinalizeParams(env, aParamInfo, aArrayType, var, aIID, PR_TRUE,
                           0, 0, i, NS_ERROR_FAILURE, nullptr);
          }
        }
      }
      PR_Free(aVariant.val.p);
      break;
    }

    case nsXPTType::T_PSTRING_SIZE_IS:
    case nsXPTType::T_PWSTRING_SIZE_IS:
    {
      NS_PRECONDITION(!aIsArrayElement, "sized string array not supported");

      if ((aParamInfo.IsOut()) && NS_SUCCEEDED(aInvokeResult))
      {
        // create new string from data
        jstring str = nullptr;
        if (aVariant.val.p) {
          if (aType == nsXPTType::T_PSTRING_SIZE_IS) {
            PRUint32 len = (aArraySize + 1) * sizeof(char);
            char* buf = (char*) moz_xmalloc(len);
            if (buf) {
              memcpy(buf, aVariant.val.p, len);
              buf[aArraySize] = '\0';
              str = env->NewStringUTF((const char*) buf);
              moz_free(buf);
            }
          } else {
            str = env->NewString((const jchar*) aVariant.val.p, aArraySize);
          }
          if (!str) {
            rv = NS_ERROR_OUT_OF_MEMORY;
            break;
          }
        }

        if (aParamInfo.IsRetval()) {
          *aParam = str;
        } else if (*aParam) {
          // put new string into output array
          env->SetObjectArrayElement((jobjectArray) *aParam, aIndex, str);
        }
      }

      // cleanup
      if (aVariant.val.p)
        moz_free(aVariant.val.p);
      break;
    }

    default:
      NS_WARNING("unexpected parameter type");
      return NS_ERROR_UNEXPECTED;
  }

  // Check for Java exception, but don't overwrite pre-existing error code.
  if (NS_SUCCEEDED(rv) && env->ExceptionCheck())
    rv = NS_ERROR_FAILURE;

  return rv;
}

nsresult
QueryAttributeInfo(nsIInterfaceInfo* aIInfo, const char* aMethodName,
                   PRBool aCapitalizedAttr, PRUint16* aMethodIndex,
                   const nsXPTMethodInfo** aMethodInfo)

{
  nsresult rv = NS_ERROR_FAILURE;

  // An 'attribute' will start with either "get" or "set".  But first,
  // we check the length, in order to skip over method names that match exactly
  // "get" or "set".
  if (strlen(aMethodName) > 3) {
    if (strncmp("get", aMethodName, 3) == 0) {
      char* getterName = strdup(aMethodName + 3);
      if (!aCapitalizedAttr) {
        getterName[0] = tolower(getterName[0]);
      }
      rv = aIInfo->GetMethodInfoForName(getterName, aMethodIndex, aMethodInfo);
      free(getterName);
    } else if (strncmp("set", aMethodName, 3) == 0) {
      char* setterName = strdup(aMethodName + 3);
      if (!aCapitalizedAttr) {
        setterName[0] = tolower(setterName[0]);
      }
      rv = aIInfo->GetMethodInfoForName(setterName, aMethodIndex, aMethodInfo);
      if (NS_SUCCEEDED(rv)) {
        // If this succeeded, GetMethodInfoForName will have returned the
        // method info for the 'getter'.  We want the 'setter', so increase
        // method index by one ('setter' immediately follows the 'getter'),
        // and get its method info.
        (*aMethodIndex)++;
        rv = aIInfo->GetMethodInfo(*aMethodIndex, aMethodInfo);
        if (NS_SUCCEEDED(rv)) {
          // Double check that this methodInfo matches the given method.
          if (!(*aMethodInfo)->IsSetter() ||
              strcmp(setterName, (*aMethodInfo)->name) != 0) {
            rv = NS_ERROR_FAILURE;
          }
        }
      }
      free(setterName);
    }
  }

  return rv;
}

/**
 * Given an interface info struct and a method name, returns the method info
 * and index, if that method exists.
 *
 * Most method names are lower case.  Unfortunately, the method names of some
 * interfaces (such as nsIAppShell) start with a capital letter.  This function
 * will try all of the permutations.
 */
nsresult
QueryMethodInfo(nsIInterfaceInfo* aIInfo, const char* aMethodName,
                PRUint16* aMethodIndex, const nsXPTMethodInfo** aMethodInfo)
{
  // Skip over any leading underscores, since these are methods that conflicted
  // with existing Java keywords
  const char* methodName = aMethodName;
  if (methodName[0] == '_') {
    methodName++;
  }

  // The common case is that the method name is lower case, so we check
  // that first.
  nsresult rv;
  rv = aIInfo->GetMethodInfoForName(methodName, aMethodIndex, aMethodInfo);
  if (NS_SUCCEEDED(rv))
    return rv;

  // If there is no method called <aMethodName>, then maybe it is an
  // 'attribute'.
  rv = QueryAttributeInfo(aIInfo, methodName, PR_FALSE, aMethodIndex,
                          aMethodInfo);
  if (NS_SUCCEEDED(rv))
    return rv;

  // If we get here, then maybe the method name is capitalized.
  char* name = strdup(methodName);
  name[0] = toupper(name[0]);
  rv = aIInfo->GetMethodInfoForName(name, aMethodIndex, aMethodInfo);
  free(name);
  if (NS_SUCCEEDED(rv))
    return rv;

  // If there is no method called <aMethodName>, then maybe it is an
  // 'attribute'.
  rv = QueryAttributeInfo(aIInfo, methodName, PR_TRUE, aMethodIndex,
                          aMethodInfo);

  return rv;
}

/**
 *  org.mozilla.xpcom.XPCOMJavaProxy.internal.callXPCOMMethod
 */
extern "C" NS_EXPORT jobject JNICALL
JAVAPROXY_NATIVE(callXPCOMMethod) (JNIEnv *env, jclass that, jobject aJavaProxy,
                                   jstring aMethodName, jobjectArray aParams)
{
  nsresult rv;

  // Get native XPCOM instance
  void* xpcom_obj;
  rv = GetXPCOMInstFromProxy(env, aJavaProxy, &xpcom_obj);
  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failed to get matching XPCOM object");
    return nullptr;
  }
  JavaXPCOMInstance* inst = static_cast<JavaXPCOMInstance*>(xpcom_obj);

  // Get method info
  PRUint16 methodIndex;
  const nsXPTMethodInfo* methodInfo;
  nsIInterfaceInfo* iinfo = inst->InterfaceInfo();
  const char* methodName = env->GetStringUTFChars(aMethodName, nullptr);
  rv = QueryMethodInfo(iinfo, methodName, &methodIndex, &methodInfo);
  env->ReleaseStringUTFChars(aMethodName, methodName);

  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "GetMethodInfoForName failed");
    return nullptr;
  }

#ifdef DEBUG_JAVAXPCOM
  const char* ifaceName;
  iinfo->GetNameShared(&ifaceName);
  LOG(("===> (XPCOM) %s::%s()\n", ifaceName, methodInfo->GetName()));
#endif

  // Convert the Java params
  PRUint8 paramCount = methodInfo->GetParamCount();
  nsXPTCVariant* params = nullptr;
  if (paramCount)
  {
    params = new nsXPTCVariant[paramCount];
    if (!params) {
      ThrowException(env, NS_ERROR_OUT_OF_MEMORY, "Can't create params array");
      return nullptr;
    }
    memset(params, 0, paramCount * sizeof(nsXPTCVariant));

    PRBool foundDependentParam = PR_FALSE;
    for (PRUint8 i = 0; i < paramCount && NS_SUCCEEDED(rv); i++)
    {
      LOG(("\t Param %d: ", i));
      const nsXPTParamInfo &paramInfo = methodInfo->GetParam(i);
      params[i].type = paramInfo.GetType();

      if (params[i].type.IsDependent() && paramInfo.IsIn()) {
        foundDependentParam = PR_TRUE;
        continue;
      }
      
      if (paramInfo.IsIn()) {
        PRUint8 type = params[i].type.TagPart();

        // get IID for interface params
        nsID iid;
        if (type == nsXPTType::T_INTERFACE) {
          rv = GetIIDForMethodParam(iinfo, methodInfo, paramInfo, type,
                                    methodIndex, params, PR_TRUE, iid);
        }

        if (NS_SUCCEEDED(rv)) {
          jobject param = nullptr;
          if (aParams && !paramInfo.IsRetval()) {
            param = env->GetObjectArrayElement(aParams, i);
          }
          rv = SetupParams(env, param, type, paramInfo.IsOut(), iid, 0, 0,
                           PR_FALSE, 0, params[i]);
        }
      } else {
        LOG(("out/retval\n"));
		params[i].SetIndirect();
        //params[i].ptr = &(params[i].val);
        //params[i].SetPtrIsData();
      }
    }
    
    // Handle any dependent params by doing a second pass
    if (foundDependentParam) {
      
      for (PRUint8 j = 0; j < paramCount && NS_SUCCEEDED(rv); j++) {
        
        const nsXPTParamInfo &paramInfo = methodInfo->GetParam(j);
        params[j].type = paramInfo.GetType();

        if (!params[j].type.IsDependent())
          continue;

        if (paramInfo.IsIn()) {
          PRUint8 type = params[j].type.TagPart();

          // is paramater an array or sized string?
          PRUint8 arrayType = 0;
          PRUint32 arraySize = 0;
          PRBool isArray = params[j].type.IsArray();
          PRBool isSizedString = isArray ? PR_FALSE :
                   type == nsXPTType::T_PSTRING_SIZE_IS ||
                   type == nsXPTType::T_PWSTRING_SIZE_IS;

          if (isArray) {
            // get array type
            nsXPTType xpttype;
            rv = iinfo->GetTypeForParam(methodIndex, &paramInfo, 1, &xpttype);
            if (NS_FAILED(rv))
              break;
            arrayType = xpttype.TagPart();
            // IDL 'octet' arrays are not 'promoted' to short, but kept as 'byte';
            // therefore, treat as a signed 8bit value
            if (arrayType == nsXPTType::T_U8)
              arrayType = nsXPTType::T_I8;
          }

          if (isArray || isSizedString) {
            // get size of array or string
            PRUint8 argnum;
            rv = iinfo->GetSizeIsArgNumberForParam(methodIndex, &paramInfo, 0,
                                                   &argnum);
            if (NS_FAILED(rv))
              break;
            arraySize = params[argnum].val.u32;
          }

          // get IID for interface params
          nsID iid;
          if (type == nsXPTType::T_INTERFACE_IS ||
              type == nsXPTType::T_ARRAY &&
                (arrayType == nsXPTType::T_INTERFACE ||
                 arrayType == nsXPTType::T_INTERFACE_IS))
          {
            PRUint8 paramType = type == nsXPTType::T_ARRAY ? arrayType : type;
            rv = GetIIDForMethodParam(iinfo, methodInfo, paramInfo, paramType,
                                      methodIndex, params, PR_TRUE, iid);
          }

          if (NS_SUCCEEDED(rv)) {
            jobject param = nullptr;
            if (aParams && !paramInfo.IsRetval()) {
              param = env->GetObjectArrayElement(aParams, j);
            }
            rv = SetupParams(env, param, type, paramInfo.IsOut(), iid, arrayType,
                             arraySize, PR_FALSE, 0, params[j]);
          }
        }
      }
    }
    
    if (NS_FAILED(rv)) {
      ThrowException(env, rv, "SetupParams failed");
      return nullptr;
    }
  }

  // Call the XPCOM method
  const nsIID* iid;
  iinfo->GetIIDShared(&iid);
  nsISupports* realObject;
  rv = inst->GetInstance()->QueryInterface(*iid, (void**) &realObject);
  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "Failed to get real XPCOM object");
    return nullptr;
  }
  nsresult invokeResult = NS_InvokeByIndex(realObject, methodIndex,
                                           paramCount, params);
  NS_RELEASE(realObject);

  // Clean up params
  jobject result = nullptr;
  for (PRUint8 i = 0; i < paramCount && NS_SUCCEEDED(rv); i++)
  {
    const nsXPTParamInfo &paramInfo = methodInfo->GetParam(i);
    PRUint8 type = paramInfo.GetType().TagPart();

    // is paramater an array or sized string?
    PRUint8 arrayType = 0;
    PRUint32 arraySize = 0;
    PRBool isArray = params[i].type.IsArray();
    PRBool isSizedString = isArray ? PR_FALSE :
             type == nsXPTType::T_PSTRING_SIZE_IS ||
             type == nsXPTType::T_PWSTRING_SIZE_IS;

    if (isArray) {
      // get array type
      nsXPTType array_xpttype;
      rv = iinfo->GetTypeForParam(methodIndex, &paramInfo, 1, &array_xpttype);
      if (NS_FAILED(rv))
        break;
      arrayType = array_xpttype.TagPart();
      // IDL 'octet' arrays are not 'promoted' to short, but kept as 'byte';
      // therefore, treat as a signed 8bit value
      if (arrayType == nsXPTType::T_U8)
        arrayType = nsXPTType::T_I8;
    }

    if (isArray || isSizedString) {
      // get size of array
      PRUint8 argnum;
      rv = iinfo->GetSizeIsArgNumberForParam(methodIndex, &paramInfo, 0,
                                             &argnum);
      if (NS_FAILED(rv))
        break;
      arraySize = params[argnum].val.u32;
    }

    // get IID for interface params
    nsID iid;
    if (type == nsXPTType::T_INTERFACE || type == nsXPTType::T_INTERFACE_IS ||
        type == nsXPTType::T_ARRAY && (arrayType == nsXPTType::T_INTERFACE ||
                                       arrayType == nsXPTType::T_INTERFACE_IS))
    {
      PRUint8 paramType = type == nsXPTType::T_ARRAY ? arrayType : type;
      rv = GetIIDForMethodParam(iinfo, methodInfo, paramInfo, paramType,
                                methodIndex, params, PR_TRUE, iid);
      if (NS_FAILED(rv))
        break;
    }

    jobject* javaElement;
    if (!paramInfo.IsRetval()) {
      jobject element = env->GetObjectArrayElement(aParams, i);
      javaElement = &element;
    } else {
      javaElement = &result;
    }
    rv = FinalizeParams(env, paramInfo, type, params[i], iid, PR_FALSE,
                        arrayType, arraySize, 0, invokeResult, javaElement);
  }

  // Normally, we would delete any created nsID object in the above loop.
  // However, GetIIDForMethodParam may need some of the nsID params when it's
  // looking for the IID of an INTERFACE_IS.  Therefore, we can't delete it
  // until we've gone through the 'Finalize' loop once and created the result.
  for (PRUint8 j = 0; j < paramCount; j++)
  {
    const nsXPTParamInfo &paramInfo = methodInfo->GetParam(j);
    const nsXPTType &type = paramInfo.GetType();
    if (type.TagPart() == nsXPTType::T_IID) {
      nsID* iid = (nsID*) params[j].val.p;
      delete iid;
    }
  }

  if (params) {
    delete params;
  }

  // If the XPCOM method invocation failed, we don't immediately throw an
  // exception and return so that we can clean up any parameters.
  if (NS_FAILED(invokeResult)) {
    nsEmbedCString message("The function \"");
    message.AppendASCII(methodInfo->GetName());
    message.AppendLiteral("\" returned an error condition");
    ThrowException(env, invokeResult, message.get());
  }
  if (NS_FAILED(rv)) {
    ThrowException(env, rv, "FinalizeParams failed");
    return nullptr;
  }

  LOG(("<=== (XPCOM) %s::%s()\n", ifaceName, methodInfo->GetName()));
  return result;
}

nsresult
GetNewOrUsedJavaWrapper(JNIEnv* env, nsISupports* aXPCOMObject,
                        const nsIID& aIID, jobject aObjectLoader,
                        jobject* aResult)
{
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  // Get the root nsISupports of the xpcom object
  nsresult rv;
  nsCOMPtr<nsISupports> rootObject = do_QueryInterface(aXPCOMObject, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get associated Java object from hash table
  rv = gNativeToJavaProxyMap->Find(env, rootObject, aIID, aResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (*aResult)
    return NS_OK;

  // No Java object is associated with the given XPCOM object, so we
  // create a Java proxy.

  nsCOMPtr<nsIInterfaceInfoManager>
    iim(do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));
  NS_ASSERTION(iim, "Failed to get InterfaceInfoManager");
  if (!iim)
    return NS_ERROR_FAILURE;

  // Get interface info for class
  nsCOMPtr<nsIInterfaceInfo> info;
  rv = iim->GetInfoForIID(&aIID, getter_AddRefs(info));
  if (NS_FAILED(rv))
    return rv;

  // Wrap XPCOM object (addrefs rootObject)
  JavaXPCOMInstance* inst = new JavaXPCOMInstance(rootObject, info);
  if (!inst)
    return NS_ERROR_OUT_OF_MEMORY;

  // Get interface name
  const char* iface_name;
  rv = info->GetNameShared(&iface_name);

  if (NS_SUCCEEDED(rv)) {
    jobject java_obj = nullptr;

    // Create proper Java interface name
    nsEmbedCString class_name("org.mozilla.interfaces.");
    class_name.AppendASCII(iface_name);
    jclass ifaceClass = FindClassInLoader(env, aObjectLoader, class_name.get());

    if (ifaceClass) {
      java_obj = env->CallStaticObjectMethod(xpcomJavaProxyClass,
                                             createProxyMID, ifaceClass,
                                             reinterpret_cast<jlong>(inst));
      if (env->ExceptionCheck())
        java_obj = nullptr;
    }

    if (java_obj) {
#ifdef DEBUG_JAVAXPCOM
      char* iid_str = aIID.ToString();
      LOG(("+ CreateJavaProxy (Java=%08x | XPCOM=%08x | IID=%s)\n",
           (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                               java_obj),
           (PRUint32) rootObject.get(), iid_str));
      NS_Free(iid_str);
#endif

      // Associate XPCOM object with Java proxy
      rv = gNativeToJavaProxyMap->Add(env, rootObject, aIID, java_obj);
      if (NS_SUCCEEDED(rv)) {
        *aResult = java_obj;
        return NS_OK;
      }
    } else {
      rv = NS_ERROR_FAILURE;
    }
  }

  // If there was an error, clean up.
  delete inst;
  return rv;
}

nsresult
GetXPCOMInstFromProxy(JNIEnv* env, jobject aJavaObject, void** aResult)
{
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  jlong xpcom_obj = env->CallStaticLongMethod(xpcomJavaProxyClass,
                                            getNativeXPCOMInstMID, aJavaObject);

  if (!xpcom_obj || env->ExceptionCheck()) {
    return NS_ERROR_FAILURE;
  }

  *aResult = reinterpret_cast<void*>(xpcom_obj);
#ifdef DEBUG_JAVAXPCOM
  JavaXPCOMInstance* inst = static_cast<JavaXPCOMInstance*>(*aResult);
  nsIID* iid;
  inst->InterfaceInfo()->GetInterfaceIID(&iid);
  char* iid_str = iid->ToString();
  LOG(("< GetXPCOMInstFromProxy (Java=%08x | XPCOM=%08x | IID=%s)\n",
       (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                           aJavaObject),
       (PRUint32) inst->GetInstance(), iid_str));
  NS_Free(iid_str);
  moz_free(iid);
#endif
  return NS_OK;
}

/**
 *  org.mozilla.xpcom.internal.XPCOMJavaProxy.finalizeProxy
 */
extern "C" NS_EXPORT void JNICALL
JAVAPROXY_NATIVE(finalizeProxy) (JNIEnv *env, jclass that, jobject aJavaProxy)
{
#ifdef DEBUG_JAVAXPCOM
  PRUint32 xpcom_addr = 0;
#endif

  // Due to Java's garbage collection, this finalize statement may get called
  // after FreeJavaGlobals().  So check to make sure that everything is still
  // initialized.
  if (gJavaXPCOMLock) {
    nsAutoLock lock(gJavaXPCOMLock);

    // If may be possible for the lock to be acquired here when FreeGlobals is
    // in the middle of running.  If so, then this thread will sleep until
    // FreeGlobals releases its lock.  At that point, we resume this thread
    // here, but JavaXPCOM may no longer be initialized.  So we need to check
    // that everything is legit after acquiring the lock.
    if (gJavaXPCOMInitialized) {
      // Get native XPCOM instance
      void* xpcom_obj;
      nsresult rv = GetXPCOMInstFromProxy(env, aJavaProxy, &xpcom_obj);
      if (NS_SUCCEEDED(rv)) {
        JavaXPCOMInstance* inst = static_cast<JavaXPCOMInstance*>(xpcom_obj);
#ifdef DEBUG_JAVAXPCOM
        xpcom_addr = reinterpret_cast<PRUint32>(inst->GetInstance());
#endif
        nsIID* iid;
        rv = inst->InterfaceInfo()->GetInterfaceIID(&iid);
        if (NS_SUCCEEDED(rv)) {
          rv = gNativeToJavaProxyMap->Remove(env, inst->GetInstance(), *iid);
          moz_free(iid);
        }
        NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to RemoveJavaProxy");
        // Release gJavaXPCOMLock before deleting inst (see bug 340022)
        lock.unlock();
        delete inst;
      }
    }
  }

#ifdef DEBUG_JAVAXPCOM
  LOG(("- Finalize (Java=%08x | XPCOM=%08x)\n",
       (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                           aJavaProxy),
       xpcom_addr));
#endif
}

/**
 *  org.mozilla.xpcom.XPCOMJavaProxy.isSameXPCOMObject
 */
extern "C" NS_EXPORT jboolean JNICALL
JAVAPROXY_NATIVE(isSameXPCOMObject) (JNIEnv *env, jclass that,
                                     jobject aProxy1, jobject aProxy2)
{
  void* xpcom_obj1;
  nsresult rv = GetXPCOMInstFromProxy(env, aProxy1, &xpcom_obj1);
  if (NS_SUCCEEDED(rv)) {
    void* xpcom_obj2;
    rv = GetXPCOMInstFromProxy(env, aProxy2, &xpcom_obj2);
    if (NS_SUCCEEDED(rv)) {
      JavaXPCOMInstance* inst1 = static_cast<JavaXPCOMInstance*>(xpcom_obj1);
      JavaXPCOMInstance* inst2 = static_cast<JavaXPCOMInstance*>(xpcom_obj2);
      if (inst1->GetInstance() == inst2->GetInstance()) {
        return JNI_TRUE;
      }
    }
  }
  return JNI_FALSE;
}

/**
 *  org.mozilla.xpcom.ProfileLock.release
 */
extern "C" NS_EXPORT void JNICALL
LOCKPROXY_NATIVE(release) (JNIEnv *env, jclass that, jlong aLockObject)
{
  // Need to release object on the main thread.
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsIThread> thread = do_GetMainThread();
  if (thread) {
    rv = NS_ProxyRelease(thread, reinterpret_cast<nsISupports*>(aLockObject));
  }
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to release using NS_ProxyRelease");
}
