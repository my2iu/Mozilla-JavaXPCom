package org.mozilla.xpcom;

/**
 * The Mozilla class seems to inherit from IXPCOMError which contains
 * many error codes. This file is usually generated based on some C header
 * files, but I'm too lazy to do that, so I'll just stub it out for now. 
 */
public interface IXPCOMError
{
   public static final long NS_ERROR_FAILURE = 0x80004005L;
}
