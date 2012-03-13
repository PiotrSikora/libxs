MAN3 = xs_bind.3 xs_close.3 xs_connect.3 xs_init.3 \
    xs_msg_close.3 xs_msg_copy.3 xs_msg_data.3 xs_msg_init.3 \
    xs_msg_init_data.3 xs_msg_init_size.3 xs_msg_move.3 xs_msg_size.3 \
    xs_poll.3 xs_recv.3 xs_send.3 xs_setsockopt.3 xs_socket.3 \
    xs_strerror.3 xs_term.3 xs_version.3 xs_getsockopt.3 xs_errno.3 \
    xs_sendmsg.3 xs_recvmsg.3 xs_getmsgopt.3 xs_setctxopt.3
MAN7 = xs.7 xs_tcp.7 xs_pgm.7 xs_inproc.7 xs_ipc.7

if BUILD_LIBZMQ
MAN7 += zmq.7
endif

MAN_DOC = $(MAN1) $(MAN3) $(MAN7)

MAN_TXT = $(MAN3:%.3=%.txt)
MAN_TXT += $(MAN7:%.7=%.txt)
MAN_HTML = $(MAN_TXT:%.txt=%.html)

if INSTALL_MAN
dist_man_MANS = $(MAN_DOC)
endif

EXTRA_DIST = asciidoc.conf $(MAN_TXT)
if BUILD_DOC
EXTRA_DIST += $(MAN_HTML)
endif

MAINTAINERCLEANFILES = $(MAN_DOC) $(MAN_HTML)

dist-hook : $(MAN_DOC) $(MAN_HTML)

if BUILD_DOC
SUFFIXES=.html .txt .xml .3 .7

.txt.html:
	asciidoc -d manpage -b xhtml11 -f $(srcdir)/asciidoc.conf \
		-axs_version=@PACKAGE_VERSION@ -o$@ $<
.txt.xml:
	asciidoc -d manpage -b docbook -f $(srcdir)/asciidoc.conf \
		-axs_version=@PACKAGE_VERSION@ -o$@ $<
.xml.1:
	xmlto man $<
.xml.3:
	xmlto man $<
.xml.7:
	xmlto man $<
endif
