/*
 * support.c
 *
 * Copyright (c) 1996, 1997
 *	Transvirtual Technologies, Inc.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution 
 * of this file. 
 */

#include "config.h"
#include "config-std.h"
#include "config-io.h"
#include "debug.h"
#include "jtypes.h"
#include "jsyscall.h"
#include "gtypes.h"
#include "gc.h"
#include "constants.h"
#include "file.h"
#include "files.h"
#include "access.h"
#include "classMethod.h"
#include "readClass.h"
#include "jar.h"
#include "kaffeh-support.h"
#include "utf8const.h"

#if !defined(S_ISDIR)
#define	S_ISDIR(m)	((m) & S_IFDIR)
#endif

#if defined(__WIN32__) || defined (__amigaos__)
#define	PATHSEP	';'
#else
#define	PATHSEP	':'
#endif

extern char realClassPath[];
extern char className[];
extern FILE* include;
extern FILE* jni_include;
extern int flag_jni;

static int objectDepth = -1;
static int outputField = 1; /* Output object fields?  Only skipped on java.lang.Class and java.lang.Object */

struct _Collector;

static inline int
binary_open(const char *file, int mode, int perm, int *);

/* circular list containing all native methods of the class that's currently being processed (sorted by name) */
struct _methodRing {
	struct _methodRing* next;
	struct _methodRing* prev;

	const char* name;
	const char* sig;
	u2 access_flags;
	bool needs_mangled_sig;
} *methodRing;

static int 
kread(int fd, void *buf, size_t len, ssize_t *out)
{
	*out = read(fd, buf, len);
	return (*out == -1) ? errno : 0;
}

static int 
kwrite(int fd, const void *buf, size_t len, ssize_t *out)
{
	*out = write(fd, buf, len);
	return (*out == -1) ? errno : 0;
}

static int
klseek(int fd, off_t off, int whence, off_t *out)
{
	*out = lseek(fd, off, whence);
	return (*out == -1) ? errno : 0;
}

/* With Tru64, stat and fstat() are silly macros, convert them to functions.  */
#if defined(stat)
static int
kstat (const char *file_name, struct stat *buf)
{
    return stat (file_name, buf);
}
#else
#define kstat	stat
#endif

#if defined(fstat)
static int
kfstat (int fd, struct stat *buf)
{
    return fstat (fd, buf);
}
#else
#define kfstat	fstat
#endif


/*
 * We use a very simple 'fake' threads subsystem
 */

SystemCallInterface Kaffe_SystemCallInterface =
{
	binary_open,
        kread,
        kwrite, 
        klseek,
        close,
        kfstat,
        kstat,

        NULL,		/* mkdir */
        NULL,		/* rmdir */
        NULL,		/* rename */
        NULL,		/* remove */
        NULL,		/* socket */
        NULL,		/* connect */
        NULL,		/* bind */
        NULL,		/* listen */
        NULL,		/* accept */
        NULL,		/* sockread */
        NULL,		/* recvfrom */
        NULL,		/* sockwrite */
        NULL,		/* sendto */
        NULL,		/* setsockopt */
        NULL,		/* getsockopt */
        NULL,		/* getsockname */
        NULL,		/* getpeername */
        NULL,		/* sockclose */
        NULL,		/* gethostbyname */
        NULL,		/* gethostbyaddr */
        NULL,		/* select */
        NULL,		/* forkexec */
        NULL,		/* waitpid */
        NULL,		/* kill */
};


extern struct _Collector* main_collector; /* in mem.c */

/* 
 * Ensure that files are opened in binary mode; the MS-Windows port
 * depends on this.
 */
static inline int
binary_open(const char *file, int mode, int perm, int *out) {
  *out = open(file, mode | O_BINARY, perm);
  return *out == -1 ? errno : 0;
}

/*
 * Init include file.
 */
void
initInclude(void)
{
	if (include == 0) {
		return;
	}

	fprintf(include, "/* DO NOT EDIT THIS FILE - it is machine generated */\n");
	fprintf(include, "#include <native.h>\n");
	fprintf(include, "\n");
	fprintf(include, "#ifndef _Included_%s\n", className);
	fprintf(include, "#define _Included_%s\n", className);
	fprintf(include, "\n");
	fprintf(include, "#ifdef __cplusplus\n");
	fprintf(include, "extern \"C\" {\n");
	fprintf(include, "#endif\n");
}

/*
 * Start include file.
 */
void
startInclude(void)
{
	if (include == 0) {
		return;
	}

	fprintf(include, "\n");
	fprintf(include, "/* Header for class %s */\n", className);
	fprintf(include, "\n");
	if ((strcmp (className, "java_lang_Object") == 0) ||
	    (strcmp (className, "java_lang_Class") == 0)) {
		outputField = 0;
	}
	else {
		outputField = 1;
		fprintf(include, "typedef struct H%s {\n", className);
		fprintf(include, "  /* Fields from java/lang/Object: */\n");
		fprintf(include, "  Hjava_lang_Object base;\n");
	}
}

/*
 * End include file.
 */
void
endInclude(void)
{
	if (include == 0) {
		return;
	}

	fprintf(include, "\n");
	fprintf(include, "#ifdef __cplusplus\n");
	fprintf(include, "}\n");
	fprintf(include, "#endif\n");
	fprintf(include, "\n");
	fprintf(include, "#endif\n");
}

void
initJniInclude(void)
{
	if (jni_include == 0) {
		return;
	}

	fprintf(jni_include, "/* DO NOT EDIT THIS FILE - it is machine generated */\n");
	fprintf(jni_include, "#include <jni.h>\n");
	fprintf(jni_include, "\n");
	fprintf(jni_include, "#ifndef _Included_%s\n", className);
	fprintf(jni_include, "#define _Included_%s\n", className);
	fprintf(jni_include, "\n");
	fprintf(jni_include, "#ifdef __cplusplus\n");
	fprintf(jni_include, "extern \"C\" {\n");
	fprintf(jni_include, "#endif\n");
	fprintf(jni_include, "\n");
}

void
startJniInclude(void)
{
}

void
endJniInclude(void)
{
	if (jni_include == 0) {
		return;
	}

	fprintf(jni_include, "\n");
	fprintf(jni_include, "#ifdef __cplusplus\n");
	fprintf(jni_include, "}\n");
	fprintf(jni_include, "#endif\n");
	fprintf(jni_include, "\n");
	fprintf(jni_include, "#endif\n");
}


/*
 * Add a class.
 */
void
addClass(u2 this, u2 super, u2 access, constants* cpool)
{
}


bool
addSourceFile(Hjava_lang_Class* c, int idx, errorInfo *einfo)
{
	return true;
}

bool
addInnerClasses(Hjava_lang_Class* c, uint32 len, classFile* fp,
		errorInfo *einfo)
{
	/* checkBufSize() done in caller. */
			seekm(fp, len);
	return true;
}

/*
 * Return the JNI type
 */
static const char *
jniType(const char *sig)
{
	switch (sig[0]) {
	case '[':
		switch (sig[1]) {
		case 'Z':
			return "jbooleanArray";
		case 'B':
			return "jbyteArray";
		case 'C':
			return "jcharArray";
		case 'S':
			return "jshortArray";
		case 'I':
			return "jintArray";
		case 'J':
			return "jlongArray";
		case 'F':
			return "jfloatArray";
		case 'D':
			return "jdoubleArray";
		case 'L':
			return "jobjectArray";
		default:
			dprintf("bogus array type `%c'", sig[1]);
			exit(1);
		}
	case 'L':
		if (strncmp(sig, "Ljava/lang/Class;", 17) == 0)
			return "jclass";
		if (strncmp(sig, "Ljava/lang/String;", 18) == 0)
			return "jstring";
		return "jobject";
	case 'I':
		return "jint";
	case 'Z':
		return "jboolean";
	case 'S':
		return "jshort";
	case 'B':
		return "jbyte";
	case 'C':
		return "jchar";
	case 'F':
		return "jfloat";
	case 'J':
		return "jlong";
	case 'D':
		return "jdouble";
	case 'V':
		return "void";
	default:
		dprintf("bogus signature type `%c'", sig[0]);
		exit(1);
	}
}

/*
 * Print a properly mangled method name or argument signature.  
 */
static void
fprintfJni (FILE *f, const char *s) 
{
	while (*s && *s != ')')
	{
		switch (*s)
		{
			case '/':
				fprintf (f, "_");
				break;
      
			case '_':
				fprintf (f, "_1");
				break;
      
			case ';':
				fprintf (f, "_2");
				break;
      
			case '[':
				fprintf (f, "_3");
				break;
			
			case '$':
				fprintf (f, "_00024");
				break;

			default:
				fprintf (f, "%c", *s);
				break;
		}
		s++;
	}
}

int
startFields(Hjava_lang_Class* this, u2 fct, errorInfo *einfo)
{
	this->fields = malloc(fct * sizeof(Field));
	this->nfields = 0; /* incremented by addField() */
	return true;
}

Field*
addField(Hjava_lang_Class* this,
	 u2 access_flags, u2 name_index, u2 signature_index,
	 struct _errorInfo* einfo)
{
	Field* f;

	if (CLASS_CONST_TAG(this, name_index) != CONSTANT_Utf8) {
		dprintf("addField(): no method name.\n"); /* XXX */
		return (0);
	}
	if (CLASS_CONST_TAG(this, signature_index) != CONSTANT_Utf8) {
		dprintf("addField(): no signature name.\n"); /* XXX */
		return (0);
	}

	DBG(CLASSFILE,
		dprintf("addField(%s.%s)\n",
			CLASS_CNAME(this), CLASS_CONST_UTF8(this, name_index)->data);
		);

	f = &(this->fields[this->nfields++]);

	/*
	 * Store enough info for the field attribute "ConstantValue" handler (setFieldValue)
	 * to print the field name/signature/access.
	 */
	f->name = CLASS_CONST_UTF8(this, name_index);
	f->type = (Hjava_lang_Class*)CLASS_CONST_UTF8(this, signature_index);
	f->accflags = access_flags;
	f->bsize = 0; /* not used by kaffeh */
	f->info.idx = 0; /* not used by kaffeh */

	if (include != 0) {
		/*
		 * Non-static fields are represented in the struct.
		 */
		if ((!(access_flags & ACC_STATIC)
		     && outputField)) {
			const char* arg;
			int argsize = 0;

			arg = translateSig(CLASS_CONST_UTF8(this, signature_index)->data,
					   NULL, &argsize);
			fprintf(include, "  %s %s;\n",
				arg, CLASS_CONST_UTF8(this, name_index)->data);
		}
	}

	return f;
}


static void
constValueToString(Hjava_lang_Class* this, u2 idx,
		   char *cval, int cvalsize)
{
	/* XXX use snprintf() */

	/* Pull the constant value for this field out of the constant pool */
	switch (CLASS_CONST_TAG(this, idx)) {
	case CONSTANT_Integer:
		sprintf(cval, "%d", (int)CLASS_CONST_DATA(this, idx));
		break;
	case CONSTANT_Float:
		sprintf(cval, "%.7e", *(float*)&CLASS_CONST_DATA(this,idx));
		break;
	case CONSTANT_Long:
#if SIZEOF_VOIDP == 8
		sprintf(cval, "0x%016lx", CLASS_CONST_DATA(this,idx));
#else
#if defined(WORDS_BIGENDIAN)
		sprintf(cval, "0x%08x%08x", CLASS_CONST_DATA(this,idx), CLASS_CONST_DATA(this,idx+1));
#else
		sprintf(cval, "0x%08x%08x", CLASS_CONST_DATA(this,idx+1), CLASS_CONST_DATA(this,idx));
#endif
#endif
		break;
	case CONSTANT_Double:
	{
		union { jint i[2]; jdouble d; } u;
		u.i[0] = CLASS_CONST_DATA(this,idx);
		u.i[1] = CLASS_CONST_DATA(this,idx+1);
		sprintf(cval, "%.16e", u.d);
		break;
	}
	case CONSTANT_String:
		/* Note, readConstantPool() puts the Utf8 pointer right into the constant pool for us. */
		sprintf(cval, "\"%s\"", CLASS_CONST_UTF8(this, idx)->data);
		break;
	default:
		sprintf(cval, "?unsupported type tag %d?", CLASS_CONST_TAG(this, idx));
		break;
	}
}


void
setFieldValue(Hjava_lang_Class* this, Field* f, u2 idx)
{
	assert(f);

	if ((f->accflags & (ACC_STATIC|ACC_PUBLIC|ACC_FINAL)) == (ACC_STATIC|ACC_PUBLIC|ACC_FINAL)) {
		char cval[512];
			
		constValueToString(this, idx, cval, sizeof(cval));

		if (cval[0] != 0) {
			if (include != 0) {
				fprintf(include, "#define %s_%s %s\n",
					className, f->name->data, cval);
			}
			if (jni_include != 0) {
				fprintf(jni_include, "#define %s_%s %s\n",
					className, f->name->data, cval);
			}
		}
	}
}

void
finishFields(Hjava_lang_Class* this)
{
	if (include == 0) {
		return;
	}

	if (objectDepth == 0) {
		if (outputField) {
			fprintf(include, "} H%s;\n\n", className);
		}
	}
}

int
startMethods(Hjava_lang_Class* this, u2 mct, errorInfo *einfo)
{
	return true;
}

Method*
addMethod(Hjava_lang_Class* this,
	  u2 access_flags, u2 name_index, u2 signature_index,
	  struct _errorInfo* einfo)
{
	constants* cpool;
	const char* name;
	const char* sig;
        struct _methodRing* list;
	struct _methodRing* i;

	/* If we shouldn't generate method prototypes, quit now */
	if (objectDepth > 0) {
		/* XXX set einfo */
		return 0;
	}

	assert(this);
	assert(einfo);

	cpool = CLASS_CONSTANTS(this);

	if (cpool->tags[name_index] != CONSTANT_Utf8) {
		dprintf("addMethod(): no method name.\n"); /* XXX */
		return (0);
	}
	if (cpool->tags[signature_index] != CONSTANT_Utf8) {
		dprintf("addMethod(): no signature name.\n"); /* XXX */
		return (0);
	}

	name = WORD2UTF(cpool->data[name_index])->data;
	sig = WORD2UTF(cpool->data[signature_index])->data;

	DBG(CLASSFILE,
		dprintf("addMethod: %s%s%s\n", name, sig,
			(access_flags & ACC_NATIVE) ? " (native)" : "");
		);

	/* Only generate stubs for native methods */
	if (!(access_flags & ACC_NATIVE)) {
		/* Return "success"... */
		return (Method*)1;
	}

	/* add the method into the list of native methods of this class */
	list = (struct _methodRing *)malloc (sizeof (struct _methodRing)); 

        list->name = name;
        list->sig  = sig;
	list->access_flags = access_flags;
	list->needs_mangled_sig = false;

	if (methodRing == NULL) {
		methodRing = list;
		
		list->next = list->prev = list;
	} else {
		i = methodRing;

		do {
			
			if (!strcmp (list->name, i->name)) {
				/* iff the names are equal, both need a mangled sig */
				list->needs_mangled_sig = true;
				i->needs_mangled_sig = true;
			  
				/* insert list */
				i->next->prev = list;
				list->next = i->next;

				i->next = list;
				list->prev = i;

				/* return success */
				return (Method*)1;	
			}
	
			i = i->next;
	
		} while (i != methodRing);
	
		/* if we didn't find a suitable place, add it at the end of the list */	
		if (i == methodRing) {
			  i->prev->next = list;
			  list->prev = i->prev;

			  i->prev = list;
			  list->next = i;
		}
	}

	return (Method*)1;
}

void
finishMethods (Hjava_lang_Class *this)
{
	const char *str;
	const char *tsig;
	struct _methodRing* i;
	struct _methodRing* tmp;

	/* don't do anything if there aren't any methods */
	if (methodRing == NULL) {
		return;
	}

	i = methodRing;
	do {
		int args = 0;
		char* ret;

	/* Generate method prototype */
		ret = strchr(i->sig,')');
	ret++;

	if (include != 0) {
		fprintf(include, "extern %s", translateSig(ret, 0, 0));
			fprintf(include, " %s_%s(", className, i->name);
			if (!(i->access_flags & ACC_STATIC)) {
			fprintf(include, "struct H%s*", className);
				if (i->sig[1] != ')') {
				fprintf(include, ", ");
			}
			} else if (i->sig[1] == ')') {
			fprintf(include, "void");
		}
	}

	if (jni_include != 0) {
			fprintf(jni_include, "JNIEXPORT %s JNICALL Java_%s_",
				jniType(ret), className);
			
			fprintfJni(jni_include, i->name);

			/* append mangled sig if necessary */
			if (i->needs_mangled_sig) {
				fprintf(jni_include, "__");
				
				fprintfJni(jni_include, i->sig+1);
			}

			fprintf(jni_include, "(JNIEnv*");

			if ((i->access_flags & ACC_STATIC)) {
			fprintf(jni_include, ", jclass");
		}
		else {
			fprintf(jni_include, ", jobject");
		}
	}

		str = i->sig + 1;
	args++;
	while (str[0] != ')') {
		if (jni_include != 0)
			fprintf(jni_include, ", %s", jniType(str));
		tsig = translateSig(str, &str, &args);
		if (include != 0) {
			fprintf(include, "%s", tsig);
			if (str[0] != ')') {
				fprintf(include, ", ");
			}
		}
	}
	if (include != 0) {
		fprintf(include, ");\n");
	}
	if (jni_include != 0) {
		fprintf(jni_include, ");\n");
	}

		/* move to next method */
		tmp = i;
		i = i->next;

		/* free old one */
		free (tmp);

	} while (i != methodRing); 

        /* we don't have any methods any more */	
	methodRing = NULL;
}

bool 
addCode(Method* m, uint32 len, classFile* fp, errorInfo *einfo)
{
	/* Don't try dereferencing m! */
	assert(m == (Method*)1);

	/* checkBufSize() done in caller. */
	seekm(fp, len);
	return true;
}

bool
addLineNumbers(Method* m, uint32 len, classFile* fp, errorInfo *info)
{
	/* Don't try dereferencing m! */
	assert(m == (Method*)1);

	/* checkBufSize() done in caller. */
	seekm(fp, len);
	return true;
}

bool
addCheckedExceptions(Method* m, uint32 len, classFile* fp,
		     errorInfo *info)
{
	/* Don't try dereferencing m! */
	assert(m == (Method*)1);

	/* checkBufSize() done in caller. */
	seekm(fp, len);
	return true;
}


Hjava_lang_Class*
setupClass(Hjava_lang_Class* this, u2 thisidx, u2 super, u2 access_flags,
	   struct Hjava_lang_ClassLoader* loader, struct _errorInfo* einfo)
{
	utf8ConstAssign(this->name, CLASS_CONST_UTF8(this, thisidx));
	this->accflags = access_flags;
	this->loader = loader;
	this->this_index = thisidx;
	this->state = CSTATE_LOADED; /* XXX */
	
	/* everything else is already zero'd. */

	if (super != 0) {
		kaffeh_findClass(CLASS_CONST_UTF8(this, super)->data);
		if (include != 0)
		{
			/* Put a blank line between fields for each (super)class. */
			fprintf(include, "\n");
		}
	}

	if (include != 0) {
		if (strcmp(CLASS_CNAME(this), "java/lang/Object")) {
			fprintf(include, "  /* Fields from %s: */\n", CLASS_CNAME(this));
		}
	}
	
	return this;
}

void
addInterfaces(Hjava_lang_Class* this, u2 icount, Hjava_lang_Class** ifaces)
{
}


/*
 * Read and process a method.
 */
void
readMethod(classFile* fp, Hjava_lang_Class* this, constants* cpool)
{
	assert(0);
}

/*
 * Locate class specified and process it.
 */
void
kaffeh_findClass(const char* nm)
{
	int fd;
	jarFile* jfile;
	jarEntry* jentry;
	char superName[512];
	struct stat sbuf;
	char* start;
	char* end = (char*)1;

	errorInfo einfo; /* XXX initialize, and check! */

	/* If classpath isn't set, get it from the environment */
	if (realClassPath[0] == 0) {
		start = getenv("KAFFE_CLASSPATH");
		if (start == 0) {
			start = getenv("CLASSPATH");
		}
		if (start == 0) {
			dprintf("CLASSPATH not set!\n");
			exit(1);
		}
		strcpy(realClassPath, start);
	}

	for (start = realClassPath; end != 0; start = end + 1) {
		end = strchr(start, PATHSEP);
		if (end == 0) {
			strcpy(superName, start);
		}
		else {
			strncpy(superName, start, end-start);
			superName[end-start] = 0;
		}

		if (stat(superName, &sbuf) < 0) {
			/* Ignore */
		}
		else if (S_ISDIR(sbuf.st_mode)) {
			Hjava_lang_Class tmpClass;
			classFile hand;
			char* buf;

			strcat(superName, "/");
			strcat(superName, nm);
			strcat(superName, ".class");
			fd = open(superName, O_RDONLY|O_BINARY, 0);
			if (fd < 0) {
				continue;
			}
			if (fstat(fd, &sbuf) < 0) {
				close(fd);
				continue;
			}

			buf = malloc(sbuf.st_size);
			if (buf == 0)
			{
				dprintf("kaffeh: Ran out of memory!");
				close(fd);
				exit(1);
			}

			/* XXX this is a bit weak. */
			if (read(fd, buf, sbuf.st_size) != sbuf.st_size) {
				free(buf);
				close(fd);
				continue;
			}

			classFileInit(&hand, buf, sbuf.st_size, CP_DIR);

			objectDepth++;
			//savepool = constant_pool;

			memset(&tmpClass, 0, sizeof(tmpClass));
			readClass(&tmpClass, &hand, NULL, &einfo);

			/* constant_pool = savepool; */
			objectDepth--;

			hand.type = CP_INVALID;
			free(buf);

			close(fd);
			return;
		}
		else {
			char *buf;
			classFile hand;
			Hjava_lang_Class tmpClass;
			
			/* JAR file */
			jfile = openJarFile(superName);
			if (jfile == 0) {
				continue;
			}

			strcpy(superName, nm);
			strcat(superName, ".class");

			jentry = lookupJarFile(jfile, superName);
			if (jentry == 0) {
				closeJarFile(jfile);
				continue;
			}

			buf = getDataJarFile(jfile, jentry);

			classFileInit(&hand, buf, jentry->uncompressedSize, CP_ZIPFILE);

			objectDepth++;
			//savepool = constant_pool;

			memset(&tmpClass, 0, sizeof(tmpClass));
			readClass(&tmpClass, &hand, NULL, &einfo);

			//constant_pool = savepool;
			objectDepth--;

			hand.type = CP_INVALID;
			free(buf);

			closeJarFile(jfile);
			return;
		}
	}
	dprintf("Failed to open object '%s'\n", nm);
	exit(1);
}
