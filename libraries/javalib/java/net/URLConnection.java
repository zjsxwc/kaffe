/*
 * Java core library component.
 *
 * Copyright (c) 1997, 1998
 *      Transvirtual Technologies, Inc.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file.
 */

package java.net;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.DateFormat;
import java.text.ParseException;
import java.util.Date;
import java.util.Vector;
import java.security.Permission;
import java.security.AllPermission;
import kaffe.net.DefaultFileNameMap;
import kaffe.net.DefaultStreamMap;
import kaffe.net.StreamMap;

abstract public class URLConnection
{
	private static AllPermission ALL_PERMISSION = new AllPermission();
	
	private static FileNameMap fileNameMap = new DefaultFileNameMap();
	private static StreamMap streamMap = new DefaultStreamMap();
	protected boolean allowUserInteraction = defaultAllowUserInteraction;
	protected boolean connected;
	protected boolean doInput = true;
	protected boolean doOutput;
	protected long ifModifiedSince;
	protected URL url;
	protected boolean useCaches = defaultUseCaches;
	protected Vector requestProperties = new Vector();
	private static ContentHandlerFactory factory;
	private static boolean defaultAllowUserInteraction;
	private static boolean defaultUseCaches;    

protected URLConnection (URL url) {
	this.url = url;
}

abstract public void connect() throws IOException;

public boolean getAllowUserInteraction() {
	return allowUserInteraction;
}

public Permission getPermission() throws IOException {
	return ALL_PERMISSION;
}

public Object getContent() throws IOException {
	return null;
}

public String getContentEncoding() {
	return getHeaderField("content-encoding");
}

public int getContentLength() {
	return getHeaderFieldInt("content-length", -1);
}

public String getContentType() {
	return getHeaderField("content-type");
}

public long getDate() {
	return getHeaderFieldDate("date", -1);
}

public static boolean getDefaultAllowUserInteraction() {
	return defaultAllowUserInteraction;
}

public static String getDefaultRequestProperty(String key) {
	return (null);
}

public boolean getDefaultUseCaches() {
	return defaultUseCaches;
}

public boolean getDoInput() {
	return doInput;
}

public boolean getDoOutput() {
	return doOutput;
}

public long getExpiration() {
	return getHeaderFieldDate("expiration", -1);
}

public static FileNameMap getFileNameMap() {
	return (fileNameMap);
}

public String getHeaderField(String name) {
	return (null);
}

public String getHeaderField(int n) {
	return (null);
}

public long getHeaderFieldDate(String name, long def) {
	String date = getHeaderField(name);
	
	if ( date != null ) {
		try {
			Date d = DateFormat.getDateInstance().parse(name);
			return (d.getTime());
		}
		catch (ParseException _) {
		}
	}

	return (def);
}

public int getHeaderFieldInt(String name, int def) {
	String val = getHeaderField(name);

	if ( val != null ) {
		try {
			return (Integer.parseInt(val));
		}
		catch (NumberFormatException _) {
		}
	}
	
	return (def);
}

public String getHeaderFieldKey(int n) {
	return (null);
}

public long getIfModifiedSince() {
	if (ifModifiedSince == 0) {
		ifModifiedSince = getHeaderFieldInt("If-Modified-Since", 0);
	}
	return ifModifiedSince;
}

public InputStream getInputStream() throws IOException {
	throw new UnknownServiceException();
}

public long getLastModified() {
	return getHeaderFieldDate("lastModified", -1);
}

public OutputStream getOutputStream() throws IOException {
	throw new UnknownServiceException();
}

public String getRequestProperty(String key) {
	return (null);
}

public URL getURL() {
	return url;
}

public boolean getUseCaches() {
	return useCaches;
}

protected static String guessContentTypeFromName(String fname)
{
	if (fileNameMap != null) {
		return (fileNameMap.getContentTypeFor(fname));
	}
	return (null);
}

public static String guessContentTypeFromStream(InputStream is) throws IOException {
	if (streamMap != null) {
		return (streamMap.getContentTypeFor(is));
	}
	return (null);
}

public void setAllowUserInteraction(boolean allowuserinteraction) {
	allowUserInteraction = allowuserinteraction;
}

/* Factory and stream type stuff */
public static synchronized void setContentHandlerFactory(ContentHandlerFactory fac) {
	if (factory == null) {
		factory = fac;
	}
	else {
		throw new Error("factory already defined");
	}
}

public static void setDefaultAllowUserInteraction(boolean defaultallowuserinteraction) {
	defaultAllowUserInteraction = defaultallowuserinteraction;
}

public static void setDefaultRequestProperty(String key, String value) {
}

public void setDefaultUseCaches(boolean defaultusecaches) {
	defaultUseCaches = defaultusecaches;
}

public void setDoInput(boolean doinput) {
	doInput = doinput;
}

public void setDoOutput(boolean dooutput) {
	doOutput = dooutput;
}

public static void setFileNameMap(FileNameMap filenameMap) {
        fileNameMap = filenameMap;
}

public void setIfModifiedSince(long ifmodifiedsince) {
	ifModifiedSince = ifmodifiedsince;
}

public void setRequestProperty(String key, String value) {
	if ( key == null )
		throw new NullPointerException("key is null");

	// In the api sun says, that http should append recurrend headers
	// but jdk 1.4.1 does not. So we replace here.
	int pos = requestProperties.indexOf( key );
	if (pos > -1) {
	        requestProperties.removeElementAt( pos+1 );
		requestProperties.removeElementAt( pos );
	}         

	requestProperties.add( key );
	requestProperties.add( value );
}

public void setUseCaches(boolean usecaches) {
	useCaches = usecaches;
}

public String toString() {
	return (getClass().toString() + " " + url);
}
}
