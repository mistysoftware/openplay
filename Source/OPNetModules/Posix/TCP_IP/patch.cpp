--- tcp_module_communication.cpp.orig	Sun Sep  8 00:15:28 2002
+++ tcp_module_communication.cpp	Mon Oct 28 14:41:19 2002
@@ -50,6 +50,9 @@
 	#include <pthread.h>
 #endif
 
+// this hacks in minimal support for EAGAIN for asynchronous communications
+#define HACKY_EAGAIN
+
 // -------------------------------  Private Definitions
 
 //since we are multithreaded, we have to use a mutual-exclusion locks for certain items
@@ -638,8 +641,20 @@
 
 		if (result == -1)
 		{
+#ifdef HACKY_EAGAIN
+			if ( errno == EAGAIN )
+			{
+				result = 0;
+			}
+			else
+			{
+				DEBUG_NETWORK_API("send",result);
+				return(kNMInternalErr);
+			}
+#else
 			DEBUG_NETWORK_API("send",result);
 			return(kNMInternalErr);
+#endif // HACKY_EAGAIN
 		}
 
 		offset += result;
@@ -690,7 +705,9 @@
 
 	if (inEndpoint->needToDie == true)
 		return kNMNoDataErr;
-	
+#ifdef HACKY_EAGAIN
+	SetNonBlockingMode(inEndpoint->sockets[which_socket]);
+#endif // HACKY_EAGAIN
 	result = recv(inEndpoint->sockets[which_socket], (char *)ioData, *ioSize, 0);
 
 	if (result == 0)
@@ -2413,7 +2430,11 @@
 	{
 		//windows complains if the buffer isnt big enough to fit the datagram. i dont think any unixes do.
 		//this of course is not a problem
+#ifdef HACKY_EAGAIN
+		if ( (op_errno == EMSGSIZE) || (op_errno == EAGAIN) )
+#else
 		if (op_errno == EMSGSIZE)
+#endif // HACKY_EAGAIN
 			return 4; //pretend we read it and go home happy
 		DEBUG_NETWORK_API("socketReadResult",result);
 		return recvfrom(socket,(char*)buffer,sizeof(buffer),0,NULL,NULL);
