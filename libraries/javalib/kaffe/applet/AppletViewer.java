package kaffe.applet;

import java.applet.Applet;
import java.applet.AppletContext;
import java.applet.AppletStub;
import java.applet.AudioClip;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.Insets;
import java.awt.Label;
import java.awt.Menu;
import java.awt.MenuBar;
import java.awt.Panel;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.awt.event.WindowListener;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.StreamTokenizer;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Observable;
import java.util.Observer;
import java.util.StringTokenizer;
import java.util.Vector;
import kaffe.management.Classpath;

/**
 * Copyright (c) 1998
 *	Transvirtual Technologies, Inc.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution 
 * of this file. 
 *
 * @author J.Mehlitz
 */
public class AppletViewer
  extends Frame
  implements AppletStub, AppletContext, ActionListener, WindowListener
{
	Applet app;
	Vector apps = new Vector();
	String codebase = "";
	String archive = "";
	String code;
	Hashtable paramDict = new Hashtable();
	Panel client = new Panel();
	Label state = new Label();
	Dimension appletSize = new Dimension( 200, 200);

public AppletViewer ( File html) {
	super( "AppletViewer: " + html);
	
	setLayout( null);
	setMenus();
	add( "South", state);
	addWindowListener(this);

	try {
		int ttype;
		FileReader fs = new FileReader( html);
		StreamTokenizer st = new StreamTokenizer( fs);

		st.lowerCaseMode( true);
		st.ordinaryChar('/');

		while ( (ttype = st.nextToken()) != st.TT_EOF ) {
			if ( ttype == '<' ){
				ttype = st.nextToken();
				if ( ttype == st.TT_WORD ) {
					if ( st.sval.equals("applet") ) {
						parseApplet( st);
					}
					else if ( st.sval.equals( "param") ) {
						parseParam(st);
					}
				}
			}
		}

		// Add both the codebase and archive into the classpath.
		if (!codebase.equals("")) {
			Classpath.add(codebase);
		}
		if (!archive.equals("")) {
			Classpath.add(archive);
		}

		fs.close();
		Class c = Class.forName(code);
		app = (Applet) c.newInstance();
		app.setStub( this);
		apps.addElement( app);

		add( app);

		appletResize( appletSize.width, appletSize.height);
		addNotify(); // a hack, in case the applet creates Graphics during init (no joke)

		app.init();
		app.validate();
		
		app.start();

		setVisible( true);		
		showStatus( "applet started");
	}
	catch ( Exception e) {
		e.printStackTrace();
		showStatus( "cannot start applet");
		e.printStackTrace();
	}
}

public void actionPerformed ( ActionEvent e ) {
	String cmd = e.getActionCommand();
	if ( "Quit".equals( cmd)) {
		app.stop();
		app.destroy();
		dispose();
	}
	else if ( "Stop".equals( cmd)) {
		app.stop();
		showStatus( "applet stopped");
	}
	else if ( "Start".equals( cmd)) {
		app.start();
		showStatus( "applet started");
	}
}

public void appletResize ( int width, int height ) {
	Insets in = getInsets();
	int w, h;
	
	w = width + in.left + in.right + 20;
	h = height + in.top + in.bottom + getStateHeight() + 40;

	appletSize.setSize( width, height);

	setBounds( 100, 100, w, h);
}

public void doLayout () {
	Insets     in = getInsets();
	Dimension  d = getSize();
	int        sh = getStateHeight();
	
	super.doLayout();
	app.setBounds( in.left + 10, in.top + 10, appletSize.width, appletSize.height);
	
	state.setBounds( in.left, d.height - in.bottom - sh, d.width - (in.left+in.right), sh);
}

public Applet getApplet ( String name) {
	return app;
}

public AppletContext getAppletContext () {
	return this;
}

public Enumeration getApplets () {
	return apps.elements();
}

public AudioClip getAudioClip ( URL url) {
	return null;
}

public URL getCodeBase () {
	try {
		return (new URL(codebase));
	}
	catch ( MalformedURLException _ ) {
		return (null);
	}
}

public URL getDocumentBase () {
	return (getCodeBase());
}

public Image getImage ( URL url) {
	return Toolkit.getDefaultToolkit().getImage( url );
}

public String getParameter ( String name) {
	String key = name.toLowerCase();
	String val = (String)paramDict.get( key);
//System.out.println( "get: " + key + " " + val);
	return val;
}

private int getStateHeight() {
        return 2 * state.getFontMetrics( state.getFont()).getHeight();
}

public boolean isActive () {
	return true;
}

public static void main ( String[] args) {
	if ( args.length == 0)
		System.out.println( "usage: AppletViewer <html file>");
	else {
		File f = new File( args[0] );
		if ( f.exists() )
			new AppletViewer( f);
		else
			System.out.println( "file " + f + " does not exist");
	}
}

void parseApplet( StreamTokenizer st ) throws IOException {

	while ( st.nextToken() != '>' ) {
		switch (st.ttype) {
		case StreamTokenizer.TT_WORD:
			if ( st.sval.equals("codebase") ) {
				st.lowerCaseMode( false);
				st.nextToken();
				st.nextToken();
				codebase = new String( st.sval);
				st.lowerCaseMode( true);
			}
			else if ( st.sval.equals("archive") ) {
				st.lowerCaseMode(false);
				st.nextToken();
				st.nextToken();
				archive = new String( st.sval);
				st.lowerCaseMode( true);
			}
			else if ( st.sval.equals( "code") ) {
				st.lowerCaseMode( false);
				st.nextToken();
				st.nextToken();
				code = new String( st.sval);
				if (code.endsWith( ".class")) {
					code = code.substring(0, code.length() - 6);
				}
				st.lowerCaseMode( true);
			}
			else if ( st.sval.equals( "height") ) {
				st.nextToken();
				st.nextToken();
				appletSize.height = (int)st.nval;
			}
			else if ( st.sval.equals( "width") ) {
				st.nextToken();
				st.nextToken();
				appletSize.width = (int)st.nval;
			}
			break;

		default:
			break;
		}
	}
}

void parseParam( StreamTokenizer st) throws IOException {
	String key = null;
	String val = null;

	while ( st.nextToken() != '>' ) {
		if ( st.sval.equals( "name") ) {
			st.nextToken();
			st.nextToken();
			key = new String( st.sval);
		}
		else if ( st.sval.equals( "value") ) {
			st.nextToken();
			st.lowerCaseMode( false);
			st.nextToken();
			if ( st.ttype == st.TT_NUMBER)
				val = Double.toString( st.nval);
			else
				val = new String( st.sval);
			st.lowerCaseMode( true);
		}
	}

	if ( ( key != null) && ( val != null) ) {
//System.out.println( "put: " + key + " " + val);
		paramDict.put( key, val);
	}
}

void setMenus () {
	MenuBar mb = new MenuBar();

	Menu m = new Menu( "Applet");
	m.add( "Restart");
	m.add( "Reload");
	m.add( "Stop");
	m.add( "Save...");
	m.add( "Start");
	m.add( "Clone...");
	m.addSeparator();
	m.add( "Tag...");
	m.add( "Info...");
	m.add( "Edit");
	m.add( "Character Encoding");
	m.addSeparator();
	m.add( "Print...");
	m.addSeparator();
	m.add( "Properties...");
	m.addSeparator();
	m.add( "Close");
	m.add( "Quit");

	m.addActionListener( this);
	
	mb.add( m);

	setMenuBar( mb);
}

public void showDocument ( URL url) {
}

public void showDocument ( URL url, String target) {
}

public void showStatus ( String str) {
	state.setText( " " + str + " " );
}

void start() {
}

public void windowActivated ( WindowEvent evt ) {
}

public void windowClosed ( WindowEvent evt ) {
}

public void windowClosing(WindowEvent e) {
	app.stop();
	app.destroy();
	dispose();
}

public void windowDeactivated ( WindowEvent evt ) {
}

public void windowDeiconified ( WindowEvent evt ) {
}

public void windowIconified ( WindowEvent evt ) {
}

public void windowOpened ( WindowEvent evt ) {
}
}
