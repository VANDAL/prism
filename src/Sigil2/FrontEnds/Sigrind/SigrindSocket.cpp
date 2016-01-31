
/*--------------------------------------------------------------------*/
/*--- A simple program to listen for valgrind logfile data.        ---*/
/*---                                          valgrind-listener.c ---*/
/*--------------------------------------------------------------------*/

/* 
 * This file has been modified for Sigil 2.0 aka PRISM. A tool for
 * capturing communication and synchronization events in a workload.
 *
 * Mike Lui
 *   mike.lui@drexel.edu
 *
 */

/*
   This file is part of Valgrind, a dynamic binary instrumentation
   framework.

   Copyright (C) 2000-2015 Julian Seward 
      jseward@acm.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.
*/


/*---------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <iostream>

#include "Sigil2/FrontEnds.hpp"
#include "MsgBuilder.hpp"

namespace
{
/*---------------------------------------------------------------*/

/* The default allowable number of concurrent connections. */
#define  M_CONNECTIONS_DEFAULT 50
/* The maximum allowable number of concurrent connections. */
#define  M_CONNECTIONS_MAX     5000

#define  DEFAULT_LOGPORT       1500

/* The maximum allowable number of concurrent connections. */
unsigned M_CONNECTIONS = 0;

/*---------------------------------------------------------------*/

__attribute__ ((noreturn))
void panic ( const char* str )
{
   fprintf(stderr,
           "\nsigrind-listener: the "
           "'impossible' happened:\n   %s\n", str);
   exit(1);
}

__attribute__ ((noreturn))
void my_assert_fail ( const char* expr, const char* file, int line, const char* fn )
{
   fprintf(stderr,
           "\nsigrind-listener: %s:%d (%s): Assertion '%s' failed.\n",
           file, line, fn, expr );
   exit(1);
}

#undef assert

#define assert(expr)                                             \
  ((void) ((expr) ? 0 :                      \
      (my_assert_fail (#expr,                  \
                            __FILE__, __LINE__,                  \
                            __PRETTY_FUNCTION__), 0)))


/*---------------------------------------------------------------*/

/* holds the fds for connections; zero if slot not in use. */
int conn_count = 0;
int           *conn_fd;
struct pollfd *conn_pollfd;


void set_nonblocking ( int sd )
{
   int res;
   res = fcntl(sd, F_GETFL);
   res = fcntl(sd, F_SETFL, res | O_NONBLOCK);
   if (res != 0) {
      perror("fcntl failed");
      panic("set_nonblocking");
   }
}

void set_blocking ( int sd )
{
   int res;
   res = fcntl(sd, F_GETFL);
   res = fcntl(sd, F_SETFL, res & ~O_NONBLOCK);
   if (res != 0) {
      perror("fcntl failed");
      panic("set_blocking");
   }
}

int read_from_sd ( int sd, VGBufState& buf_state )
{
   char buf[100];
   int n;

   set_blocking(sd);
   n = read(sd, buf, 99);
   if (n <= 0) return 0; /* closed */
   parseSglMsg(buf, n, buf_state);

   set_nonblocking(sd);
   while (1) {
      n = read(sd, buf, 100);
      if (n <= 0) return 1; /* not closed */
      parseSglMsg(buf, n, buf_state);
   }
}


void snooze ( void )
{
   struct timespec req;
   req.tv_sec = 0;
   req.tv_nsec = 200 * 1000 * 1000;
   nanosleep(&req,NULL);
}


/* returns 0 if negative, or > BOUND or invalid characters were found */
int atoi_with_bound ( const char* str, int bound )
{
   int n = 0;
   while (1) {
      if (*str == 0) 
         break;
      if (*str < '0' || *str > '9')
         return 0;
      n = 10*n + (int)(*str - '0');
      str++;
      if (n >= bound)
         return 0;
   }
   return n;
}

/* returns 0 if invalid, else port # */
int atoi_portno ( const char* str )
{
   int n = atoi_with_bound(str, 65536);

   if (n < 1024)
      return 0;
   return n;
}


/* DEPRECATED */
void usage ( void )
{
   fprintf(stderr, 
      "\n"
      "usage is:\n"
      "\n"
      "   sigrind-listener [port-number]\n"
      "\n"
      "   where   port-number is the default port on which to listen for\n"
      "           connections.  It must be between 1024 and 65535.\n"
      "           Current default is %d.\n"
      "\n"
      ,
      DEFAULT_LOGPORT
   );
   exit(1);
}

char* const* tokenize_sigrind_opts(const std::string& user_exec)
{
	using namespace std;

	/* FIXME this does not account for quoted arguments with spaces
	 * Both whitespace and quote pairs should be delimiters */
	istringstream iss(user_exec);
	vector<string> tokens{
			istream_iterator<string>(iss),
			istream_iterator<string>()};

	//                 program name + valgrind options + user program options + null
	int vg_opts_size = 1            + 1                + tokens.size()        + 1;
	char** vg_opts = static_cast<char**>( malloc(vg_opts_size * sizeof(char*)) );

	int i = 0;
	vg_opts[i++] = strdup("valgrind");
	vg_opts[i++] = strdup("--tool=sigrind");
	for (string token : tokens) 
	{
		vg_opts[i++] = strdup(token.c_str());
	}
	vg_opts[i] = nullptr;

	return vg_opts;
}

void start_sigrind(const std::string& user_exec, const std::string& sigrind_dir)
{
	std::string vg_exec = sigrind_dir + "/valgrind";

	// execvp() expects a const char* const*
	auto vg_opts = tokenize_sigrind_opts(user_exec);
	execvp(vg_exec.c_str(), vg_opts);
}

int listen_loop(int main_sd)
{
   VGBufState buf_state;
   struct sockaddr_in client_addr;
   while (1) {

      snooze();

      /* enquire, using poll, whether there is any activity available on
         the main socket descriptor.  If so, someone is trying to
         connect; get the fd and add it to our table thereof. */
      { struct pollfd ufd;
        while (1) {
           ufd.fd = main_sd;
           ufd.events = POLLIN;
           ufd.revents = 0;
           int res = poll(&ufd, 1, 0);
           if (res == 0) break;

           /* ok, we have someone waiting to connect.  Get the sd. */
           socklen_t client_len = sizeof(client_addr);
           int new_sd = accept(main_sd, (struct sockaddr *)&client_addr, 
                                                       &client_len);
           if (new_sd < 0) {
              perror("cannot accept connection ");
              panic("main -- accept connection");
           }

           /* find a place to put it. */
      assert(new_sd > 0);
           int i;
           for (i = 0; i < M_CONNECTIONS; i++)
              if (conn_fd[i] == 0) 
                 break;

           if (i >= M_CONNECTIONS) {
              fprintf(stderr, "\n\nMore than %d concurrent connections.\n"
                      "Restart the listener giving --max-connect=INT on the\n"
                      "commandline to increase the limit.\n\n",
                      M_CONNECTIONS);
              exit(1);
           }

           conn_fd[i] = new_sd;
           conn_count++;
        } /* while (1) */
      }

      /* We've processed all new connect requests.  Listen for changes
         to the current set of fds. */
      int j = 0;
      for (int i = 0; i < M_CONNECTIONS; i++) {
         if (conn_fd[i] == 0)
            continue;
         conn_pollfd[j].fd = conn_fd[i];
         conn_pollfd[j].events = POLLIN /* | POLLHUP | POLLNVAL */;
         conn_pollfd[j].revents = 0;
         j++;
      }

      int res = poll(conn_pollfd, j, 0 /* return immediately. */ );
      if (res < 0) {
         perror("poll(main) failed");
         panic("poll(main) failed");
      }
    
      /* nothing happened. go round again. */
      if (res == 0) {
         continue;
      }

      /* inspect the fds. */
      for (int i = 0; i < j; i++) {
 
         if (conn_pollfd[i].revents & POLLIN) {
            /* data is available on this fd */
            res = read_from_sd(conn_pollfd[i].fd, buf_state);
 
            if (res == 0) {
               /* the connection has been closed. */
               close(conn_pollfd[i].fd);
               /* this fd has been closed or otherwise gone bad; forget
                 about it. */
			   int k;
               for (k = 0; k < M_CONNECTIONS; k++)
                  if (conn_fd[k] == conn_pollfd[i].fd) 
                     break;
               assert(k < M_CONNECTIONS);
               conn_fd[k] = 0;
               conn_count--;

			   char tstr[64];
               time_t t = time(NULL);
               struct tm * p = localtime(&t);
               strftime(tstr, 64, "%c", p);
               std::cerr << "\n------------------- Sigrind connection closed -------------------" << std::endl;;
               std::cerr <<   "-------------------  " << tstr << " -------------------" << std::endl;;

               if (conn_count == 0) {
                  printf("\n");
                  fflush(stdout);
				  goto finish;
          }
            }
         }

      } /* for (i = 0; i < j; i++) */
  
   } /* while (1) */

   finish: return 0;
}

} //end namespace




int sigrind_frontend (const std::string& user_exec, const std::string& sigrind_dir) 
{
   int    main_sd;
   struct sockaddr_in server_addr;

   int port = DEFAULT_LOGPORT;

   M_CONNECTIONS = M_CONNECTIONS_DEFAULT;

   conn_fd     =(int*) malloc(M_CONNECTIONS * sizeof conn_fd[0]);
   conn_pollfd = (struct pollfd*)malloc(M_CONNECTIONS * sizeof conn_pollfd[0]);
   if (conn_fd == NULL || conn_pollfd == NULL) {
      fprintf(stderr, "Memory allocation failed; cannot continue.\n");
      exit(1);
   }

   std::cerr << "Connecting to Sigrind as a separate process" << std::endl;;
   char tstr[64];
   time_t t = time(NULL);
   struct tm * p = localtime(&t);
   strftime(tstr, 64, "%c", p);
   std::cerr << "------------------- Sigrind connection opening ------------------" << std::endl;;
   std::cerr <<   "-------------------  " << tstr << " -------------------\n" << std::endl;;

   conn_count = 0;
   for (int i = 0; i < M_CONNECTIONS; i++)
      conn_fd[i] = 0;

   /* create socket */
   main_sd = socket(AF_INET, SOCK_STREAM, 0);
   if (main_sd < 0) {
      perror("cannot open socket ");
      panic("main -- create socket");
   }

   /* allow address reuse to avoid "address already in use" errors */

   int one = 1;
   if (setsockopt(main_sd, SOL_SOCKET, SO_REUSEADDR, 
        &one, sizeof(int)) < 0) {
      perror("cannot enable address reuse ");
      panic("main -- enable address reuse");
   }

   /* bind server port */
   server_addr.sin_family      = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port        = htons(port);
  
   if (bind(main_sd, (struct sockaddr *) &server_addr, 
                     sizeof(server_addr) ) < 0) {
      perror("cannot bind port ");
      panic("main -- bind port");
   }

   int res = listen(main_sd,M_CONNECTIONS);
   if (res != 0) {
      perror("listen failed ");
      panic("main -- listen");
   }


   pid_t pid;
   pid = fork();
   if ( pid >= 0 )
   {
      if ( pid == 0 )
      {
         start_sigrind(user_exec, sigrind_dir);
      }
      else
      {
         return listen_loop(main_sd);
      }
   }
   else
   {
      perror("fork failed ");
      panic("main -- fork");
   }
}
