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

import java.lang.String;
import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

abstract public class SocketImpl
  implements SocketOptions {

protected FileDescriptor fd = new FileDescriptor();
protected InetAddress address = new InetAddress();
protected int port;
protected int localport;

protected FileDescriptor getFileDescriptor() {
	return (fd);
}

protected InetAddress getInetAddress() {
	return (address);
}

protected int getLocalPort() {
	return (localport);
}

protected int getPort() {
	return (port);
}

public String toString() {
	return "ServerSocket[addr="+address.toString()+",port="+String.valueOf(port)+",localport="+String.valueOf(localport)+"]";
}

abstract protected void accept(SocketImpl s) throws IOException;
abstract protected int available() throws IOException;
abstract protected void bind(InetAddress host, int port) throws IOException;
abstract protected void close() throws IOException;
abstract protected void connect(InetAddress address, int port) throws IOException;
abstract protected void connect(String host, int port) throws IOException;
abstract protected void create(boolean stream) throws IOException;
abstract protected void listen(int backlog) throws IOException;
abstract protected OutputStream getOutputStream() throws IOException;
abstract protected InputStream getInputStream() throws IOException;

/**
 * Difficult to know why Sun didn't do it this way, but it causes problems
 * if these two are abstract as they should be.
 */
protected void write(byte[] buf, int offset, int len) throws IOException {
	throw new Error("must be define by a real socket implementation");
}

protected int read(byte[] buf, int offset, int len) throws IOException {
	throw new Error("must be define by a real socket implementation");
}

}
