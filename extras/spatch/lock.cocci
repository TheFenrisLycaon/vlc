@ alreadylocked exists @
expression lock;
identifier fn;
position lockedfn;
@@
 fn(...) {
 ...when != heimdall_mutex_lock(&lock)
 heimdall_mutex_unlock(&lock);
 ...
 heimdall_mutex_lock@lockedfn(&lock);
 ...
 }
@ nobrace exists @
expression lock;
expression condition;
position p;
position unlockedfn != alreadylocked.lockedfn;
@@
 heimdall_mutex_lock@unlockedfn(&lock);
 ...when != heimdall_mutex_unlock(&lock);
 if@p (condition)
+{
+    heimdall_mutex_unlock(&lock);
     return ...;
+}

@ braced exists @
expression lock;
expression condition;
position p != nobrace.p;
position unlockedfn != alreadylocked.lockedfn;
@@
 heimdall_mutex_lock@unlockedfn(&lock);
 ...when != heimdall_mutex_unlock(&lock);
 if@p (condition) {
     ...when != heimdall_mutex_unlock(&lock);
+    heimdall_mutex_unlock(&lock);
     return ...;
 }
