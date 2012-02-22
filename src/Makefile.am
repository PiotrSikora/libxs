lib_LTLIBRARIES = libxs.la libxszmq.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxs.pc libxszmq.pc

include_HEADERS = ../include/xs.h ../include/xs_utils.h \
    ../include/xszmq.h ../include/xszmq_utils.h

libxs_la_SOURCES = \
    array.hpp \
    atomic_counter.hpp \
    atomic_ptr.hpp \
    blob.hpp \
    clock.hpp \
    command.hpp \
    config.hpp \
    ctx.hpp \
    decoder.hpp \
    devpoll.hpp \
    dist.hpp \
    encoder.hpp \
    epoll.hpp \
    err.hpp \
    fd.hpp \
    fq.hpp \
    io_object.hpp \
    io_thread.hpp \
    ip.hpp \
    ipc_address.hpp \
    ipc_connecter.hpp \
    ipc_listener.hpp \
    i_engine.hpp \
    kqueue.hpp \
    lb.hpp \
    likely.hpp \
    mailbox.hpp \
    monitor.hpp \
    msg.hpp \
    mtrie.hpp \
    mutex.hpp \
    object.hpp \
    options.hpp \
    own.hpp \
    pgm_receiver.hpp \
    pgm_sender.hpp \
    pgm_socket.hpp \
    pipe.hpp \
    platform.hpp \
    poll.hpp \
    pair.hpp \
    pub.hpp \
    pull.hpp \
    push.hpp \
    random.hpp \
    reaper.hpp \
    rep.hpp \
    req.hpp \
    select.hpp \
    session_base.hpp \
    signaler.hpp \
    socket_base.hpp \
    stdint.hpp \
    stream_engine.hpp \
    sub.hpp \
    tcp_address.hpp \
    tcp_connecter.hpp \
    tcp_listener.hpp \
    thread.hpp \
    trie.hpp \
    upoll.hpp \
    windows.hpp \
    wire.hpp \
    xpub.hpp \
    xrep.hpp \
    xreq.hpp \
    xsub.hpp \
    ypipe.hpp \
    yqueue.hpp \
    clock.cpp \
    ctx.cpp \
    decoder.cpp \
    devpoll.cpp \
    dist.cpp \
    encoder.cpp \
    epoll.cpp \
    err.cpp \
    fq.cpp \
    io_object.cpp \
    io_thread.cpp \
    ip.cpp \
    ipc_address.cpp \
    ipc_connecter.cpp \
    ipc_listener.cpp \
    kqueue.cpp \
    lb.cpp \
    mailbox.cpp \
    monitor.cpp \
    msg.cpp \
    mtrie.cpp \
    object.cpp \
    options.cpp \
    own.cpp \
    pair.cpp \
    pgm_receiver.cpp \
    pgm_sender.cpp \
    pgm_socket.cpp \
    pipe.cpp \
    poll.cpp \
    pull.cpp \
    push.cpp \
    reaper.cpp \
    pub.cpp \
    random.cpp \
    rep.cpp \
    req.cpp \
    select.cpp \
    session_base.cpp \
    signaler.cpp \
    socket_base.cpp \
    stream_engine.cpp \
    sub.cpp \
    tcp_address.cpp \
    tcp_connecter.cpp \
    tcp_listener.cpp \
    thread.cpp \
    trie.cpp \
    upoll.cpp \
    xpub.cpp \
    xrep.cpp \
    xreq.cpp \
    xsub.cpp \
    xs.cpp \
    xs_utils.cpp

if ON_MINGW
libxs_la_LDFLAGS = -no-undefined -avoid-version -version-info @LIBXS_ABI_VERSION@ @LIBXS_EXTRA_LDFLAGS@
else
libxs_la_LDFLAGS = -version-info @LIBXS_ABI_VERSION@ @LIBXS_EXTRA_LDFLAGS@
endif

libxs_la_CXXFLAGS = @LIBXS_EXTRA_CXXFLAGS@

if BUILD_PGM
libxs_la_CPPFLAGS = -I$(top_srcdir)/@pgm_srcdir@/include/
libxs_la_LIBADD = $(top_srcdir)/@pgm_srcdir@/libpgm_noinst.la
endif

libxszmq_la_SOURCES = xszmq.c
libxszmq_la_CPPFLAGS = -DXS_BUILDING_LIBXSZMQ
libxszmq_la_LIBADD = libxs.la

if ON_MINGW
libxszmq_la_LDFLAGS = -no-undefined -avoid-version -version-info @LIBXSZMQ_ABI_VERSION@
else
libxszmq_la_LDFLAGS = -version-info @LIBXSZMQ_ABI_VERSION@
endif

dist-hook:
		-rm $(distdir)/platform.hpp


