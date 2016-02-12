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
 * The Initial Developer of the Original Code is
 * IBM Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2005
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

#include "nsJavaXPCOMBindingUtils.h"
#include "nsJavaXPTCStub.h"
#include "nsJavaWrapper.h"
#include "jni.h"
#include "nsIInterfaceInfoManager.h"
#include "nsILocalFile.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"


/* Java JNI globals */

JavaVM* gCachedJVM = nullptr;

jclass systemClass = nullptr;
jclass booleanClass = nullptr;
jclass charClass = nullptr;
jclass byteClass = nullptr;
jclass shortClass = nullptr;
jclass intClass = nullptr;
jclass longClass = nullptr;
jclass floatClass = nullptr;
jclass doubleClass = nullptr;
jclass stringClass = nullptr;
jclass nsISupportsClass = nullptr;
jclass xpcomExceptionClass = nullptr;
jclass xpcomJavaProxyClass = nullptr;
jclass weakReferenceClass = nullptr;
jclass javaXPCOMUtilsClass = nullptr;

jmethodID hashCodeMID = nullptr;
jmethodID booleanValueMID = nullptr;
jmethodID booleanInitMID = nullptr;
jmethodID charValueMID = nullptr;
jmethodID charInitMID = nullptr;
jmethodID byteValueMID = nullptr;
jmethodID byteInitMID = nullptr;
jmethodID shortValueMID = nullptr;
jmethodID shortInitMID = nullptr;
jmethodID intValueMID = nullptr;
jmethodID intInitMID = nullptr;
jmethodID longValueMID = nullptr;
jmethodID longInitMID = nullptr;
jmethodID floatValueMID = nullptr;
jmethodID floatInitMID = nullptr;
jmethodID doubleValueMID = nullptr;
jmethodID doubleInitMID = nullptr;
jmethodID createProxyMID = nullptr;
jmethodID isXPCOMJavaProxyMID = nullptr;
jmethodID getNativeXPCOMInstMID = nullptr;
jmethodID weakReferenceConstructorMID = nullptr;
jmethodID getReferentMID = nullptr;
jmethodID clearReferentMID = nullptr;
jmethodID findClassInLoaderMID = nullptr;

#ifdef DEBUG_JAVAXPCOM
jmethodID getNameMID = nullptr;
jmethodID proxyToStringMID = nullptr;
#endif

NativeToJavaProxyMap* gNativeToJavaProxyMap = nullptr;
JavaToXPTCStubMap* gJavaToXPTCStubMap = nullptr;

PRBool gJavaXPCOMInitialized = PR_FALSE;
PRLock* gJavaXPCOMLock = nullptr;

static const char* kJavaKeywords[] = {
  "abstract", "default"  , "if"        , "private"     , "throw"       ,
  "boolean" , "do"       , "implements", "protected"   , "throws"      ,
  "break"   , "double"   , "import",     "public"      , "transient"   ,
  "byte"    , "else"     , "instanceof", "return"      , "try"         ,
  "case"    , "extends"  , "int"       , "short"       , "void"        ,
  "catch"   , "final"    , "interface" , "static"      , "volatile"    ,
  "char"    , "finally"  , "long"      , "super"       , "while"       ,
  "class"   , "float"    , "native"    , "switch"      ,
  "const"   , "for"      , "new"       , "synchronized",
  "continue", "goto"     , "package"   , "this"        ,
    /* added in Java 1.2 */
  "strictfp",
    /* added in Java 1.4 */
  "assert"  ,
    /* added in Java 5.0 */
  "enum"    ,
    /* Java constants */
  "true"    , "false"    , "null"      ,
    /* java.lang.Object methods                                           *
     *    - don't worry about "toString", since it does the same thing    *
     *      as Object's "toString"                                        */
  "clone"   , "equals"   , "finalize"  , "getClass"    , "hashCode"    ,
  "notify"  , "notifyAll", /*"toString"  ,*/ "wait"
};

nsTHashtable<nsDepCharHashKey>* gJavaKeywords = nullptr;


/******************************
 *  InitializeJavaGlobals
 ******************************/
PRBool
InitializeJavaGlobals(JNIEnv *env)
{
  if (gJavaXPCOMInitialized)
    return PR_TRUE;

  // Save pointer to JavaVM, which is valid across threads.
  jint rc = env->GetJavaVM(&gCachedJVM);
  if (rc != 0) {
    NS_WARNING("Failed to get JavaVM");
    goto init_error;
  }

  jclass clazz;
  if (!(clazz = env->FindClass("java/lang/System")) ||
      !(systemClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(hashCodeMID = env->GetStaticMethodID(clazz, "identityHashCode",
                                             "(Ljava/lang/Object;)I")))
  {
    NS_WARNING("Problem creating java.lang.System globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Boolean")) ||
      !(booleanClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(booleanValueMID = env->GetMethodID(clazz, "booleanValue", "()Z")) ||
      !(booleanInitMID = env->GetMethodID(clazz, "<init>", "(Z)V")))
  {
    NS_WARNING("Problem creating java.lang.Boolean globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Character")) ||
      !(charClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(charValueMID = env->GetMethodID(clazz, "charValue", "()C")) ||
      !(charInitMID = env->GetMethodID(clazz, "<init>", "(C)V")))
  {
    NS_WARNING("Problem creating java.lang.Character globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Byte")) ||
      !(byteClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(byteValueMID = env->GetMethodID(clazz, "byteValue", "()B")) ||
      !(byteInitMID = env->GetMethodID(clazz, "<init>", "(B)V")))
  {
    NS_WARNING("Problem creating java.lang.Byte globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Short")) ||
      !(shortClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(shortValueMID = env->GetMethodID(clazz, "shortValue", "()S")) ||
      !(shortInitMID = env->GetMethodID(clazz, "<init>", "(S)V")))
  {
    NS_WARNING("Problem creating java.lang.Short globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Integer")) ||
      !(intClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(intValueMID = env->GetMethodID(clazz, "intValue", "()I")) ||
      !(intInitMID = env->GetMethodID(clazz, "<init>", "(I)V")))
  {
    NS_WARNING("Problem creating java.lang.Integer globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Long")) ||
      !(longClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(longValueMID = env->GetMethodID(clazz, "longValue", "()J")) ||
      !(longInitMID = env->GetMethodID(clazz, "<init>", "(J)V")))
  {
    NS_WARNING("Problem creating java.lang.Long globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Float")) ||
      !(floatClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(floatValueMID = env->GetMethodID(clazz, "floatValue", "()F")) ||
      !(floatInitMID = env->GetMethodID(clazz, "<init>", "(F)V")))
  {
    NS_WARNING("Problem creating java.lang.Float globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/Double")) ||
      !(doubleClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(doubleValueMID = env->GetMethodID(clazz, "doubleValue", "()D")) ||
      !(doubleInitMID = env->GetMethodID(clazz, "<init>", "(D)V")))
  {
    NS_WARNING("Problem creating java.lang.Double globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/String")) ||
      !(stringClass = (jclass) env->NewGlobalRef(clazz)))
  {
    NS_WARNING("Problem creating java.lang.String globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("org/mozilla/interfaces/nsISupports")) ||
      !(nsISupportsClass = (jclass) env->NewGlobalRef(clazz)))
  {
    NS_WARNING("Problem creating org.mozilla.interfaces.nsISupports globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("org/mozilla/xpcom/XPCOMException")) ||
      !(xpcomExceptionClass = (jclass) env->NewGlobalRef(clazz)))
  {
    NS_WARNING("Problem creating org.mozilla.xpcom.XPCOMException globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("org/mozilla/xpcom/internal/XPCOMJavaProxy")) ||
      !(xpcomJavaProxyClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(createProxyMID = env->GetStaticMethodID(clazz, "createProxy",
                                   "(Ljava/lang/Class;J)Ljava/lang/Object;")) ||
      !(isXPCOMJavaProxyMID = env->GetStaticMethodID(clazz, "isXPCOMJavaProxy",
                                                    "(Ljava/lang/Object;)Z")) ||
      !(getNativeXPCOMInstMID = env->GetStaticMethodID(xpcomJavaProxyClass,
                                                       "getNativeXPCOMInstance",
                                                       "(Ljava/lang/Object;)J")))
  {
    NS_WARNING("Problem creating org.mozilla.xpcom.internal.XPCOMJavaProxy globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("java/lang/ref/WeakReference")) ||
      !(weakReferenceClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(weakReferenceConstructorMID = env->GetMethodID(weakReferenceClass, 
                                           "<init>","(Ljava/lang/Object;)V")) ||
      !(getReferentMID = env->GetMethodID(weakReferenceClass,
                                          "get", "()Ljava/lang/Object;")) ||
      !(clearReferentMID = env->GetMethodID(weakReferenceClass, 
                                            "clear", "()V")))
  {
    NS_WARNING("Problem creating java.lang.ref.WeakReference globals");
    goto init_error;
  }

  if (!(clazz = env->FindClass("org/mozilla/xpcom/internal/JavaXPCOMMethods")) ||
      !(javaXPCOMUtilsClass = (jclass) env->NewGlobalRef(clazz)) ||
      !(findClassInLoaderMID = env->GetStaticMethodID(clazz,
                    "findClassInLoader",
                    "(Ljava/lang/Object;Ljava/lang/String;)Ljava/lang/Class;")))
  {
    NS_WARNING("Problem creating org.mozilla.xpcom.internal.JavaXPCOMMethods globals");
    goto init_error;
  }

#ifdef DEBUG_JAVAXPCOM
  if (!(clazz = env->FindClass("java/lang/Class")) ||
      !(getNameMID = env->GetMethodID(clazz, "getName","()Ljava/lang/String;")))
  {
    NS_WARNING("Problem creating java.lang.Class globals");
    goto init_error;
  }

  if (!(proxyToStringMID = env->GetStaticMethodID(xpcomJavaProxyClass,
                                                  "proxyToString",
                                     "(Ljava/lang/Object;)Ljava/lang/String;")))
  {
    NS_WARNING("Problem creating proxyToString global");
    goto init_error;
  }
#endif

  gNativeToJavaProxyMap = new NativeToJavaProxyMap();
  if (!gNativeToJavaProxyMap || NS_FAILED(gNativeToJavaProxyMap->Init())) {
    NS_WARNING("Problem creating NativeToJavaProxyMap");
    goto init_error;
  }
  gJavaToXPTCStubMap = new JavaToXPTCStubMap();
  if (!gJavaToXPTCStubMap || NS_FAILED(gJavaToXPTCStubMap->Init())) {
    NS_WARNING("Problem creating JavaToXPTCStubMap");
    goto init_error;
  }

  {
    nsresult rv = NS_OK;
    PRUint32 size = MOZ_ARRAY_LENGTH(kJavaKeywords);
    gJavaKeywords = new nsTHashtable<nsDepCharHashKey>(size);
    if (!gJavaKeywords) {// || NS_FAILED(gJavaKeywords->Init(size))) {
      NS_WARNING("Failed to init JavaKeywords HashSet");
      goto init_error;
    }
    for (PRUint32 i = 0; i < size && NS_SUCCEEDED(rv); i++) {
      if (!gJavaKeywords->PutEntry(kJavaKeywords[i])) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to populate JavaKeywords hash");
      goto init_error;
    }
  }

  gJavaXPCOMLock = nsAutoLock::NewLock("gJavaXPCOMLock");
  gJavaXPCOMInitialized = PR_TRUE;
  return PR_TRUE;

init_error:
  // If we encounter an error during initialization, then free any globals that
  // were allocated, and return false.
  FreeJavaGlobals(env);
  return PR_FALSE;
}

/*************************
 *    FreeJavaGlobals
 *************************/
void
FreeJavaGlobals(JNIEnv* env)
{
  PRLock* tempLock = nullptr;
  if (gJavaXPCOMLock) {
    PR_Lock(gJavaXPCOMLock);

    // null out global lock so no one else can use it
    tempLock = gJavaXPCOMLock;
    gJavaXPCOMLock = nullptr;
  }

  gJavaXPCOMInitialized = PR_FALSE;

  // Free the mappings first, since that process depends on some of the Java
  // globals that are freed later.
  if (gNativeToJavaProxyMap) {
    gNativeToJavaProxyMap->Destroy(env);
    delete gNativeToJavaProxyMap;
    gNativeToJavaProxyMap = nullptr;
  }
  if (gJavaToXPTCStubMap) {
    gJavaToXPTCStubMap->Destroy();
    delete gJavaToXPTCStubMap;
    gJavaToXPTCStubMap = nullptr;
  }

  // Free remaining Java globals
  if (systemClass) {
    env->DeleteGlobalRef(systemClass);
    systemClass = nullptr;
  }
  if (booleanClass) {
    env->DeleteGlobalRef(booleanClass);
    booleanClass = nullptr;
  }
  if (charClass) {
    env->DeleteGlobalRef(charClass);
    charClass = nullptr;
  }
  if (byteClass) {
    env->DeleteGlobalRef(byteClass);
    byteClass = nullptr;
  }
  if (shortClass) {
    env->DeleteGlobalRef(shortClass);
    shortClass = nullptr;
  }
  if (intClass) {
    env->DeleteGlobalRef(intClass);
    intClass = nullptr;
  }
  if (longClass) {
    env->DeleteGlobalRef(longClass);
    longClass = nullptr;
  }
  if (floatClass) {
    env->DeleteGlobalRef(floatClass);
    floatClass = nullptr;
  }
  if (doubleClass) {
    env->DeleteGlobalRef(doubleClass);
    doubleClass = nullptr;
  }
  if (stringClass) {
    env->DeleteGlobalRef(stringClass);
    stringClass = nullptr;
  }
  if (nsISupportsClass) {
    env->DeleteGlobalRef(nsISupportsClass);
    nsISupportsClass = nullptr;
  }
  if (xpcomExceptionClass) {
    env->DeleteGlobalRef(xpcomExceptionClass);
    xpcomExceptionClass = nullptr;
  }
  if (xpcomJavaProxyClass) {
    env->DeleteGlobalRef(xpcomJavaProxyClass);
    xpcomJavaProxyClass = nullptr;
  }
  if (weakReferenceClass) {
    env->DeleteGlobalRef(weakReferenceClass);
    weakReferenceClass = nullptr;
  }

  if (gJavaKeywords) {
    delete gJavaKeywords;
    gJavaKeywords = nullptr;
  }

  if (tempLock) {
    PR_Unlock(tempLock);
    nsAutoLock::DestroyLock(tempLock);
  }
}


/**************************************
 *  Java<->XPCOM object mappings
 **************************************/

// NativeToJavaProxyMap: The common case is that each XPCOM object will have
// one Java proxy.  But there are instances where there will be multiple Java
// proxies for a given XPCOM object, each representing a different interface.
// So we optimize the common case by using a hash table.  Then, if there are
// multiple Java proxies, we cycle through the linked list, comparing IIDs.

nsresult
NativeToJavaProxyMap::Init()
{
  mHashTable = PL_NewDHashTable(PL_DHashGetStubOps(), nullptr,
                                sizeof(Entry), 16);
  if (!mHashTable)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}

PLDHashOperator
DestroyJavaProxyMappingEnum(PLDHashTable* aTable, PLDHashEntryHdr* aHeader,
                            PRUint32 aNumber, void* aData)
{
  JNIEnv* env = static_cast<JNIEnv*>(aData);
  NativeToJavaProxyMap::Entry* entry =
                          static_cast<NativeToJavaProxyMap::Entry*>(aHeader);

  // first, delete XPCOM instances from the Java proxies
  nsresult rv;
  NativeToJavaProxyMap::ProxyList* item = entry->list;
  while(item != nullptr) {
    void* xpcom_obj;
    jobject javaObject = env->CallObjectMethod(item->javaObject, getReferentMID);
    rv = GetXPCOMInstFromProxy(env, javaObject, &xpcom_obj);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get XPCOM instance from Java proxy");

    if (NS_SUCCEEDED(rv)) {
      JavaXPCOMInstance* inst = static_cast<JavaXPCOMInstance*>(xpcom_obj);
#ifdef DEBUG_JAVAXPCOM
      char* iid_str = item->iid.ToString();
      LOG(("- NativeToJavaProxyMap (Java=%08x | XPCOM=%08x | IID=%s)\n",
           (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                               javaObject),
           (PRUint32) entry, iid_str));
      NS_Free(iid_str);
#endif
      delete inst;  // releases native XPCOM object
    }

    NativeToJavaProxyMap::ProxyList* next = item->next;
    env->CallVoidMethod(item->javaObject, clearReferentMID);
    env->DeleteGlobalRef(item->javaObject);
    delete item;
    item = next;
  }

  return PL_DHASH_REMOVE;
}

nsresult
NativeToJavaProxyMap::Destroy(JNIEnv* env)
{
  // This is only called from FreeGlobals(), which already holds the lock.
  //  nsAutoLock lock(gJavaXPCOMLock);

  PL_DHashTableEnumerate(mHashTable, DestroyJavaProxyMappingEnum, env);
  PL_DHashTableDestroy(mHashTable);
  mHashTable = nullptr;

  return NS_OK;
}

nsresult
NativeToJavaProxyMap::Add(JNIEnv* env, nsISupports* aXPCOMObject,
                          const nsIID& aIID, jobject aProxy)
{
  nsAutoLock lock(gJavaXPCOMLock);

  Entry* e = static_cast<Entry*>(PL_DHashTableOperate(mHashTable,
                                                         aXPCOMObject,
                                                         PL_DHASH_ADD));
  if (!e)
    return NS_ERROR_FAILURE;

  jobject ref = nullptr;
  jobject weakRefObj = env->NewObject(weakReferenceClass,
                                      weakReferenceConstructorMID, aProxy);
  if (weakRefObj)
    ref = env->NewGlobalRef(weakRefObj);
  if (!ref)
    return NS_ERROR_OUT_OF_MEMORY;

  // Add Java proxy weak reference ref to start of list
  ProxyList* item = new ProxyList(ref, aIID, e->list);
  e->key = aXPCOMObject;
  e->list = item;

#ifdef DEBUG_JAVAXPCOM
  char* iid_str = aIID.ToString();
  LOG(("+ NativeToJavaProxyMap (Java=%08x | XPCOM=%08x | IID=%s)\n",
       (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID, aProxy),
       (PRUint32) aXPCOMObject, iid_str));
  NS_Free(iid_str);
#endif
  return NS_OK;
}

nsresult
NativeToJavaProxyMap::Find(JNIEnv* env, nsISupports* aNativeObject,
                           const nsIID& aIID, jobject* aResult)
{
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (!aResult)
    return NS_ERROR_FAILURE;

  nsAutoLock lock(gJavaXPCOMLock);

  *aResult = nullptr;
  Entry* e = static_cast<Entry*>(PL_DHashTableOperate(mHashTable,
                                                         aNativeObject,
                                                         PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_FREE(e))
    return NS_OK;

  ProxyList* item = e->list;
  while (item != nullptr && *aResult == nullptr) {
    if (item->iid.Equals(aIID)) {
      jobject referentObj = env->CallObjectMethod(item->javaObject,
                                                  getReferentMID);
      if (!env->IsSameObject(referentObj, NULL)) {
        *aResult = referentObj;
#ifdef DEBUG_JAVAXPCOM
        char* iid_str = aIID.ToString();
        LOG(("< NativeToJavaProxyMap (Java=%08x | XPCOM=%08x | IID=%s)\n",
             (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                                 *aResult),
             (PRUint32) aNativeObject, iid_str));
        NS_Free(iid_str);
#endif
      }
    }
    item = item->next;
  }

  return NS_OK;
}

nsresult
NativeToJavaProxyMap::Remove(JNIEnv* env, nsISupports* aNativeObject,
                             const nsIID& aIID)
{
  // This is only called from finalizeProxy(), which already holds the lock.
  //  nsAutoLock lock(gJavaXPCOMLock);

  Entry* e = static_cast<Entry*>(PL_DHashTableOperate(mHashTable,
                                                         aNativeObject,
                                                         PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_FREE(e)) {
    NS_WARNING("XPCOM object not found in hash table");
    return NS_ERROR_FAILURE;
  }

  ProxyList* item = e->list;
  ProxyList* last = e->list;
  while (item != nullptr) {
    if (item->iid.Equals(aIID)) {
#ifdef DEBUG_JAVAXPCOM
      char* iid_str = aIID.ToString();
      LOG(("- NativeToJavaProxyMap (Java=%08x | XPCOM=%08x | IID=%s)\n",
           (PRUint32) env->CallStaticIntMethod(systemClass, hashCodeMID,
                                               item->javaObject),
           (PRUint32) aNativeObject, iid_str));
      NS_Free(iid_str);
#endif

      env->CallVoidMethod(item->javaObject, clearReferentMID);
      env->DeleteGlobalRef(item->javaObject);
      if (item == e->list) {
        e->list = item->next;
        if (e->list == nullptr)
          PL_DHashTableOperate(mHashTable, aNativeObject, PL_DHASH_REMOVE);
      } else {
        last->next = item->next;
      }

      delete item;
      return NS_OK;
    }

    last = item;
    item = item->next;
  }

  NS_WARNING("Java proxy matching given IID not found");
  return NS_ERROR_FAILURE;
}

nsresult
JavaToXPTCStubMap::Init()
{
  mHashTable = PL_NewDHashTable(PL_DHashGetStubOps(), nullptr,
                                sizeof(Entry), 16);
  if (!mHashTable)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


PLDHashOperator
DestroyXPTCMappingEnum(PLDHashTable* aTable, PLDHashEntryHdr* aHeader,
                       PRUint32 aNumber, void* aData)
{
  JavaToXPTCStubMap::Entry* entry =
                             static_cast<JavaToXPTCStubMap::Entry*>(aHeader);

  // The XPTC stub will be released by the XPCOM side, if it hasn't been
  // already.  We just need to delete the Java global ref held by the XPTC stub,
  // so the Java garbage collector can handle the Java object when necessary.
  entry->xptcstub->DeleteStrongRef();

  return PL_DHASH_REMOVE;
}

nsresult
JavaToXPTCStubMap::Destroy()
{
  // This is only called from FreeGlobals(), which already holds the lock.
  //  nsAutoLock lock(gJavaXPCOMLock);

  PL_DHashTableEnumerate(mHashTable, DestroyXPTCMappingEnum, nullptr);
  PL_DHashTableDestroy(mHashTable);
  mHashTable = nullptr;

  return NS_OK;
}

nsresult
JavaToXPTCStubMap::Add(jint aJavaObjectHashCode, nsJavaXPTCStub* aProxy)
{
  nsAutoLock lock(gJavaXPCOMLock);

  Entry* e = static_cast<Entry*>
                        (PL_DHashTableOperate(mHashTable,
                                           NS_INT32_TO_PTR(aJavaObjectHashCode),
                                           PL_DHASH_ADD));
  if (!e)
    return NS_ERROR_FAILURE;

  NS_ASSERTION(e->key == nullptr,
               "XPTCStub for given Java object already exists in hash table");
  e->key = aJavaObjectHashCode;
  e->xptcstub = aProxy;

#ifdef DEBUG_JAVAXPCOM
  nsIInterfaceInfo* iface_info;
  aProxy->GetInterfaceInfo(&iface_info);
  nsIID* iid;
  iface_info->GetInterfaceIID(&iid);
  char* iid_str = iid->ToString();
  LOG(("+ JavaToXPTCStubMap (Java=%08x | XPCOM=%08x | IID=%s)\n",
       (PRUint32) aJavaObjectHashCode, (PRUint32) aProxy, iid_str));
  NS_Free(iid_str);
  free(iid);
  NS_RELEASE(iface_info);
#endif
  return NS_OK;
}

nsresult
JavaToXPTCStubMap::Find(jint aJavaObjectHashCode, const nsIID& aIID,
                        nsJavaXPTCStub** aResult)
{
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (!aResult)
    return NS_ERROR_FAILURE;

  nsAutoLock lock(gJavaXPCOMLock);

  *aResult = nullptr;
  Entry* e = static_cast<Entry*>
                        (PL_DHashTableOperate(mHashTable,
                                           NS_INT32_TO_PTR(aJavaObjectHashCode),
                                           PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_FREE(e))
    return NS_OK;

  nsresult rv = e->xptcstub->QueryInterface(aIID, (void**) aResult);

#ifdef DEBUG_JAVAXPCOM
  if (NS_SUCCEEDED(rv)) {
    char* iid_str = aIID.ToString();
    LOG(("< JavaToXPTCStubMap (Java=%08x | XPCOM=%08x | IID=%s)\n",
         (PRUint32) aJavaObjectHashCode, (PRUint32) *aResult, iid_str));
    free(iid_str);
  }
#endif

  // NS_NOINTERFACE is not an error condition
  if (rv == NS_NOINTERFACE)
    rv = NS_OK;
  return rv;
}

nsresult
JavaToXPTCStubMap::Remove(jint aJavaObjectHashCode)
{
  PL_DHashTableOperate(mHashTable, NS_INT32_TO_PTR(aJavaObjectHashCode),
                       PL_DHASH_REMOVE);

#ifdef DEBUG_JAVAXPCOM
  LOG(("- JavaToXPTCStubMap (Java=%08x)\n", (PRUint32) aJavaObjectHashCode));
#endif

  return NS_OK;
}


/**********************************************************
 *    JavaXPCOMInstance
 *********************************************************/
JavaXPCOMInstance::JavaXPCOMInstance(nsISupports* aInstance,
                                     nsIInterfaceInfo* aIInfo)
    : mInstance(aInstance)
    , mIInfo(aIInfo)
{
  NS_ADDREF(mInstance);
  NS_ADDREF(mIInfo);
}

JavaXPCOMInstance::~JavaXPCOMInstance()
{
  nsresult rv = NS_OK;
  nsresult rv2 = NS_OK;

  // Need to release these objects on the main thread.
  nsCOMPtr<nsIThread> thread = do_GetMainThread();
  if (thread) {
    rv = NS_ProxyRelease(thread, mInstance);
    rv2 = NS_ProxyRelease(thread, mIInfo);
  }
  NS_ASSERTION(NS_SUCCEEDED(rv) && NS_SUCCEEDED(rv2), "Failed to release using NS_ProxyRelease");
}


/*******************************
 *  Helper functions
 *******************************/

nsresult
NativeInterfaceToJavaObject(JNIEnv* env, nsISupports* aXPCOMObject,
                            const nsIID& aIID, jobject aObjectLoader,
                            jobject* aResult)
{
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  // If the object is an nsJavaXPTCStub, then get the Java object directly
  nsJavaXPTCStub* stub = nullptr;
  aXPCOMObject->QueryInterface(NS_GET_IID(nsJavaXPTCStub), (void**) &stub);
  if (stub) {
    *aResult = stub->GetJavaObject();
    NS_ASSERTION(*aResult != nullptr, "nsJavaXPTCStub w/o matching Java object");
    NS_RELEASE(stub);
    return NS_OK;
  }

  // ... else, get a Java wrapper for the native object
  return GetNewOrUsedJavaWrapper(env, aXPCOMObject, aIID, aObjectLoader,
                                 aResult);
}

nsresult
JavaObjectToNativeInterface(JNIEnv* env, jobject aJavaObject, const nsIID& aIID,
                            void** aResult)
{
  NS_PRECONDITION(aResult != nullptr, "null ptr");
  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;
  *aResult = nullptr;

  // If the given Java object is one of our Java proxies, then query the
  // associated XPCOM object directly from the proxy.
  jboolean isProxy = env->CallStaticBooleanMethod(xpcomJavaProxyClass,
                                                  isXPCOMJavaProxyMID,
                                                  aJavaObject);
  if (env->ExceptionCheck())
    return NS_ERROR_FAILURE;

  if (isProxy) {
    void* inst;
    rv = GetXPCOMInstFromProxy(env, aJavaObject, &inst);
    NS_ENSURE_SUCCESS(rv, rv);

    nsISupports* rootObject =
              static_cast<JavaXPCOMInstance*>(inst)->GetInstance();
    rv = rootObject->QueryInterface(aIID, aResult);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // ... else, we get an nsJavaXPTCStub
  return nsJavaXPTCStub::GetNewOrUsed(env, aJavaObject, aIID, aResult);
}

nsresult
GetIIDForMethodParam(nsIInterfaceInfo *iinfo,
                     const XPTMethodDescriptor *methodInfo,
                     const nsXPTParamInfo &paramInfo, PRUint8 paramType,
                     PRUint16 methodIndex, nsXPTCMiniVariant *dispatchParams,
                     PRBool isFullVariantArray, nsID &result)
{
  nsresult rv;

  switch (paramType)
  {
    case nsXPTType::T_INTERFACE:
      rv = iinfo->GetIIDForParamNoAlloc(methodIndex, &paramInfo, &result);
      break;

    case nsXPTType::T_INTERFACE_IS:
    {
      PRUint8 argnum;
      rv = iinfo->GetInterfaceIsArgNumberForParam(methodIndex, &paramInfo,
                                                  &argnum);
      if (NS_FAILED(rv))
        break;

      const nsXPTParamInfo& arg_param = methodInfo->params[argnum];
      const nsXPTType& arg_type = arg_param.GetType();

      // The xpidl compiler ensures this. We reaffirm it for safety.
      if (!arg_type.deprecated_IsPointer() || arg_type.TagPart() != nsXPTType::T_IID) {
        rv = NS_ERROR_UNEXPECTED;
        break;
      }

      nsID *p = nullptr;
      if (isFullVariantArray) {
        p = (nsID *) ((nsXPTCVariant*) dispatchParams)[argnum].val.p;
      } else {
        p = (nsID *) dispatchParams[argnum].val.p;
      }
      if (!p)
        return NS_ERROR_UNEXPECTED;

      result = *p;
      break;
    }

    default:
      rv = NS_ERROR_UNEXPECTED;
  }
  return rv;
}


/*******************************
 *  JNI helper functions
 *******************************/

JNIEnv*
GetJNIEnv()
{
  JNIEnv* env;
  jint rc = gCachedJVM->GetEnv((void**) &env, JNI_VERSION_1_2);
  NS_ASSERTION(rc == JNI_OK && env != nullptr,
               "Current thread not attached to given JVM instance");
  return env;
}

void
ThrowException(JNIEnv* env, const nsresult aErrorCode, const char* aMessage)
{
  // Only throw this exception if one hasn't already been thrown, so we don't
  // mask a previous exception/error.
  if (env->ExceptionCheck())
    return;

  // If the error code we get is for an Out Of Memory error, try to throw an
  // OutOfMemoryError.  The JVM may have enough memory to create this error.
  if (aErrorCode == NS_ERROR_OUT_OF_MEMORY) {
    jclass clazz = env->FindClass("java/lang/OutOfMemoryError");
    if (clazz) {
      env->ThrowNew(clazz, aMessage);
    }
    env->DeleteLocalRef(clazz);
    return;
  }

  // If the error was not handled above, then create an XPCOMException with the
  // given error code and message.

  // Create parameters and method signature. Max of 2 params.  The error code
  // comes before the message string.
  PRInt64 errorCode = static_cast<uint32_t>(static_cast<uint32_t>(aErrorCode) ? aErrorCode : NS_ERROR_FAILURE);
  nsEmbedCString methodSig("(J");
  jstring message = nullptr;
  if (aMessage) {
    message = env->NewStringUTF(aMessage);
    if (!message) {
      return;
    }
    methodSig.AppendLiteral("Ljava/lang/String;");
  }
  methodSig.AppendLiteral(")V");

  // In some instances (such as in shutdownXPCOM() and termEmbedding()), we
  // will need to throw an exception when JavaXPCOM has already been
  // terminated.  In such a case, 'xpcomExceptionClass' will be null.  So we
  // reset it temporarily in order to throw the appropriate exception.
  if (xpcomExceptionClass == nullptr) {
    xpcomExceptionClass = env->FindClass("org/mozilla/xpcom/XPCOMException");
    if (!xpcomExceptionClass) {
      return;
    }
  }

  // create exception object
  jthrowable throwObj = nullptr;
  jmethodID mid = env->GetMethodID(xpcomExceptionClass, "<init>",
                                   methodSig.get());
  if (mid) {
    throwObj = (jthrowable) env->NewObject(xpcomExceptionClass, mid, errorCode,
                                           message);
  }
  NS_ASSERTION(throwObj, "Failed to create XPCOMException object");

  // throw exception
  if (throwObj) {
    env->Throw(throwObj);
  }
}

nsString*
jstring_to_nsString(JNIEnv* env, jstring aString)
{
  const jchar* buf = nullptr;
  if (aString) {
    buf = env->GetStringChars(aString, nullptr);
    if (!buf)
      return nullptr;  // exception already thrown
  }

  nsString* str = new nsString((PRUnichar *)buf);

  if (aString) {
    env->ReleaseStringChars(aString, buf);
  } else {
    str->SetIsVoid(PR_TRUE);
  }

  // returns string, or nullptr if 'new' failed
  return str;
}

nsCString*
jstring_to_nsCString(JNIEnv* env, jstring aString)
{
  const char* buf = nullptr;
  if (aString) {
    buf = env->GetStringUTFChars(aString, nullptr);
    if (!buf)
      return nullptr;  // exception already thrown
  }

  nsCString* str = new nsCString(buf);

  if (aString) {
    env->ReleaseStringUTFChars(aString, buf);
  } else {
    str->SetIsVoid(PR_TRUE);
  }

  // returns string, or nullptr if 'new' failed
  return str;
}

nsresult
File_to_nsILocalFile(JNIEnv* env, jobject aFile, nsIFile** aLocalFile)
{
  nsresult rv = NS_ERROR_FAILURE;
  jstring pathName = nullptr;
  jclass clazz = env->FindClass("java/io/File");
  if (clazz) {
    jmethodID pathMID = env->GetMethodID(clazz, "getCanonicalPath",
                                         "()Ljava/lang/String;");
    if (pathMID) {
      pathName = (jstring) env->CallObjectMethod(aFile, pathMID);
      if (pathName != nullptr && !env->ExceptionCheck())
        rv = NS_OK;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    nsString* path = jstring_to_nsString(env, pathName);
    if (!path)
      rv = NS_ERROR_OUT_OF_MEMORY;

    if (NS_SUCCEEDED(rv)) {
      rv = NS_NewLocalFile(*path, false, aLocalFile);
      delete path;
    }
  }

  return rv;
}

