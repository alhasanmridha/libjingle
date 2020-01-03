#
# libjingle
# Copyright 2012, Google Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  3. The name of the author may not be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

{
  'includes': ['build/common.gypi'],
  'targets': [
    {
      # TODO(ronghuawu): Use gtest.gyp from chromium.
      'target_name': 'gunit',
      'type': 'static_library',
      'sources': [
        '<(DEPTH)/testing/gtest/src/gtest-all.cc',
      ],
      'include_dirs': [
        '<(DEPTH)/testing/gtest/include',
        '<(DEPTH)/testing/gtest',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)/testing/gtest/include',
        ],
      },
      'conditions': [
        ['OS=="android"', {
          'include_dirs': [
            '<(android_ndk_include)',
          ]
        }],
      ],
    },  # target gunit
    {
      'target_name': 'libjingle_unittest_main',
      'type': 'static_library',
      'dependencies': [
        '<(DEPTH)/third_party/libyuv/libyuv.gyp:libyuv',
        'gunit',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)/third_party/libyuv/include',
        ],
      },
      'sources': [
        'unittest_main.cc',
        # Also use this as a convenient dumping ground for misc files that are
        # included by multiple targets below.
        'fakecpumonitor.h',
        'fakenetwork.h',
        'fakesslidentity.h',
        'faketaskrunner.h',
        'gunit.h',
        'testbase64.h',
        'testechoserver.h',
        'win32toolhelp.h',
        'media/fakecapturemanager.h',
        'media/fakemediaengine.h',
        'media/fakemediaprocessor.h',
        'media/fakenetworkinterface.h',
        'media/fakertp.h',
        'media/fakevideocapturer.h',
        'media/fakevideorenderer.h',
        'media/nullvideoframe.h',
        'media/nullvideorenderer.h',
        'media/testutils.cc',
        'media/testutils.h',
        'media/devices/fakedevicemanager.h',
        'media/webrtc/fakewebrtccommon.h',
        'media/webrtc/fakewebrtcdeviceinfo.h',
        'media/webrtc/fakewebrtcvcmfactory.h',
        'media/webrtc/fakewebrtcvideocapturemodule.h',
        'media/webrtc/fakewebrtcvideoengine.h',
        'media/webrtc/fakewebrtcvoiceengine.h',
      ],
    },  # target libjingle_unittest_main
    {
      'target_name': 'libjingle_unittest',
      'type': 'executable',
      'dependencies': [
        'gunit',
        'libjingle.gyp:libjingle',
        'libjingle_unittest_main',
      ],
      'sources': [
        'asynchttprequest_unittest.cc',
        'atomicops_unittest.cc',
        'autodetectproxy_unittest.cc',
        'bandwidthsmoother_unittest.cc',
        'base64_unittest.cc',
        'basictypes_unittest.cc',
        'bind_unittest.cc',
        'buffer_unittest.cc',
        'bytebuffer_unittest.cc',
        'byteorder_unittest.cc',
        'cpumonitor_unittest.cc',
        'crc32_unittest.cc',
        'event_unittest.cc',
        'filelock_unittest.cc',
        'fileutils_unittest.cc',
        'helpers_unittest.cc',
        'host_unittest.cc',
        'httpbase_unittest.cc',
        'httpcommon_unittest.cc',
        'httpserver_unittest.cc',
        'ipaddress_unittest.cc',
        'logging_unittest.cc',
        'md5digest_unittest.cc',
        'messagedigest_unittest.cc',
        'messagequeue_unittest.cc',
        'multipart_unittest.cc',
        'nat_unittest.cc',
        'network_unittest.cc',
        'nullsocketserver_unittest.cc',
        'optionsfile_unittest.cc',
        'pathutils_unittest.cc',
        'physicalsocketserver_unittest.cc',
        'profiler_unittest.cc',
        'proxy_unittest.cc',
        'proxydetect_unittest.cc',
        'ratelimiter_unittest.cc',
        'ratetracker_unittest.cc',
        'referencecountedsingletonfactory_unittest.cc',
        'rollingaccumulator_unittest.cc',
        'sha1digest_unittest.cc',
        'sharedexclusivelock_unittest.cc',
        'signalthread_unittest.cc',
        'sigslot_unittest.cc',
        'socket_unittest.cc',
        'socket_unittest.h',
        'socketaddress_unittest.cc',
        'stream_unittest.cc',
        'stringencode_unittest.cc',
        'stringutils_unittest.cc',
        # TODO(ronghuawu): Reenable this test.
        # 'systeminfo_unittest.cc',
        'task_unittest.cc',
        'testclient_unittest.cc',
        'thread_unittest.cc',
        'timeutils_unittest.cc',
        'urlencode_unittest.cc',
        'versionparsing_unittest.cc',
        'virtualsocket_unittest.cc',
        # TODO(ronghuawu): Reenable this test.
        # 'windowpicker_unittest.cc',
        'xmllite/qname_unittest.cc',
        'xmllite/xmlbuilder_unittest.cc',
        'xmllite/xmlelement_unittest.cc',
        'xmllite/xmlnsstack_unittest.cc',
        'xmllite/xmlparser_unittest.cc',
        'xmllite/xmlprinter_unittest.cc',
        'xmpp/fakexmppclient.h',
        'xmpp/hangoutpubsubclient_unittest.cc',
        'xmpp/jid_unittest.cc',
        'xmpp/mucroomconfigtask_unittest.cc',
        'xmpp/mucroomdiscoverytask_unittest.cc',
        'xmpp/mucroomlookuptask_unittest.cc',
        'xmpp/mucroomuniquehangoutidtask_unittest.cc',
        'xmpp/pingtask_unittest.cc',
        'xmpp/pubsubclient_unittest.cc',
        'xmpp/pubsubtasks_unittest.cc',
        'xmpp/util_unittest.cc',
        'xmpp/util_unittest.h',
        'xmpp/xmppengine_unittest.cc',
        'xmpp/xmpplogintask_unittest.cc',
        'xmpp/xmppstanzaparser_unittest.cc',
      ],  # sources
      'conditions': [
        ['OS=="linux"', {
          'sources': [
            'latebindingsymboltable_unittest.cc',
            # TODO(ronghuawu): Reenable this test.
            # 'linux_unittest.cc',
            'linuxfdwalk_unittest.cc',
          ],
        }],
        ['OS=="win"', {
          'sources': [
            'win32_unittest.cc',
            'win32regkey_unittest.cc',
            'win32socketserver_unittest.cc',
            'win32toolhelp_unittest.cc',
            'win32window_unittest.cc',
            'win32windowpicker_unittest.cc',
            'winfirewall_unittest.cc',
          ],
          'sources!': [
            # TODO(ronghuawu): Fix TestUdpReadyToSendIPv6 on windows bot
            # then reenable these tests.
            'physicalsocketserver_unittest.cc',
            'socket_unittest.cc',
            'win32socketserver_unittest.cc',
            'win32windowpicker_unittest.cc',
          ],
        }],
        ['OS=="mac"', {
          'sources': [
            'macsocketserver_unittest.cc',
            'macutils_unittest.cc',
            'macwindowpicker_unittest.cc',
          ],
        }],
        ['os_posix==1', {
          'sources': [
            'sslidentity_unittest.cc',
            'sslstreamadapter_unittest.cc',
          ],
        }],
      ],  # conditions
    },  # target libjingle_unittest
    {
      'target_name': 'libjingle_sound_unittest',
      'type': 'executable',
      'dependencies': [
        'gunit',
        'libjingle.gyp:libjingle_sound',
        'libjingle_unittest_main',
      ],
      'sources': [
        'sound/automaticallychosensoundsystem_unittest.cc',
      ],
    },  # target libjingle_sound_unittest
    {
      'target_name': 'libjingle_media_unittest',
      'type': 'executable',
      'dependencies': [
        'gunit',
        'libjingle.gyp:libjingle_media',
        'libjingle_unittest_main',
      ],
      # TODO(ronghuawu): Avoid the copies.
      # https://code.google.com/p/libjingle/issues/detail?id=398
      'copies': [
        {
          'destination': '<(DEPTH)/../talk/media/testdata',
          'files': [
            'media/testdata/1.frame_plus_1.byte',
            'media/testdata/captured-320x240-2s-48.frames',
            'media/testdata/h264-svc-99-640x360.rtpdump',
            'media/testdata/video.rtpdump',
            'media/testdata/voice.rtpdump',
          ],
        },
      ],
      'sources': [
        # TODO(ronghuawu): Reenable this test.
        # 'media/capturemanager_unittest.cc',
        'media/codec_unittest.cc',
        'media/filemediaengine_unittest.cc',
        'media/rtpdataengine_unittest.cc',
        'media/rtpdump_unittest.cc',
        'media/rtputils_unittest.cc',
        'media/testutils.cc',
        'media/testutils.h',
        'media/videocapturer_unittest.cc',
        'media/videocommon_unittest.cc',
        'media/videoengine_unittest.h',
        'media/devices/dummydevicemanager_unittest.cc',
        'media/devices/filevideocapturer_unittest.cc',
        'media/webrtc/webrtcpassthroughrender_unittest.cc',
        'media/webrtc/webrtcvideocapturer_unittest.cc',
        # Omitted because depends on non-open-source testdata files.
        # 'media/videoframe_unittest.h',
        # 'media/webrtc/webrtcvideoframe_unittest.cc',

        # Disabled because some tests fail.
        # TODO(ronghuawu): Reenable these tests.
        # 'media/devices/devicemanager_unittest.cc',
        # 'media/webrtc/webrtcvideoengine_unittest.cc',
        # 'media/webrtc/webrtcvoiceengine_unittest.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalDependencies': [
                # TODO(ronghuawu): Since we've included strmiids in
                # libjingle_media target, we shouldn't need this here.
                # Find out why it doesn't work without this.
                'strmiids.lib',
              ],
            },
          },
        }],
      ],
    },  # target libjingle_media_unittest
    {
      'target_name': 'libjingle_p2p_unittest',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/third_party/libsrtp/libsrtp.gyp:libsrtp',
        'gunit',
        'libjingle.gyp:libjingle',
        'libjingle.gyp:libjingle_p2p',
        'libjingle_unittest_main',
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/libsrtp/srtp',
      ],
      'sources': [
        'p2p/dtlstransportchannel_unittest.cc',
        'p2p/fakesession.h',
        'p2p/p2ptransportchannel_unittest.cc',
        'p2p/port_unittest.cc',
        'p2p/portallocatorsessionproxy_unittest.cc',
        'p2p/pseudotcp_unittest.cc',
        'p2p/relayport_unittest.cc',
        'p2p/relayserver_unittest.cc',
        'p2p/session_unittest.cc',
        'p2p/stun_unittest.cc',
        'p2p/stunport_unittest.cc',
        'p2p/stunrequest_unittest.cc',
        'p2p/stunserver_unittest.cc',
        'p2p/testrelayserver.h',
        'p2p/teststunserver.h',
        'p2p/testturnserver.h',
        'p2p/transport_unittest.cc',
        'p2p/transportdescriptionfactory_unittest.cc',
        'p2p/client/connectivitychecker_unittest.cc',
        'p2p/client/fakeportallocator.h',
        'p2p/client/portallocator_unittest.cc',
        'session/media/channel_unittest.cc',
        'session/media/channelmanager_unittest.cc',
        'session/media/currentspeakermonitor_unittest.cc',
        'session/media/mediarecorder_unittest.cc',
        'session/media/mediamessages_unittest.cc',
        'session/media/mediasession_unittest.cc',
        'session/media/mediasessionclient_unittest.cc',
        'session/media/rtcpmuxfilter_unittest.cc',
        'session/media/srtpfilter_unittest.cc',
        'session/media/ssrcmuxfilter_unittest.cc',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_settings': {
            'VCLinkerTool': {
              'AdditionalDependencies': [
                'strmiids.lib',
              ],
            },
          },
        }],
      ],
    },  # target libjingle_p2p_unittest
    {
      'target_name': 'libjingle_peerconnection_unittest',
      'type': 'executable',
      'dependencies': [
        'gunit',
        'libjingle.gyp:libjingle',
        'libjingle.gyp:libjingle_p2p',
        'libjingle.gyp:libjingle_peerconnection',
        'libjingle_unittest_main',
      ],
      # TODO(ronghuawu): Reenable below unit tests that require gmock.
      'sources': [
        'app/webrtc/dtmfsender_unittest.cc',
        'app/webrtc/jsepsessiondescription_unittest.cc',
        'app/webrtc/localaudiosource_unittest.cc',
        'app/webrtc/localvideosource_unittest.cc',
        # 'app/webrtc/mediastream_unittest.cc',
        # 'app/webrtc/mediastreamhandler_unittest.cc',
        'app/webrtc/mediastreamsignaling_unittest.cc',
        'app/webrtc/peerconnection_unittest.cc',
        'app/webrtc/peerconnectionfactory_unittest.cc',
        'app/webrtc/peerconnectioninterface_unittest.cc',
        # 'app/webrtc/peerconnectionproxy_unittest.cc',
        'app/webrtc/test/fakeaudiocapturemodule.cc',
        'app/webrtc/test/fakeaudiocapturemodule.h',
        'app/webrtc/test/fakeaudiocapturemodule_unittest.cc',
        'app/webrtc/test/fakeconstraints.h',
        'app/webrtc/test/fakeperiodicvideocapturer.h',
        'app/webrtc/test/fakevideotrackrenderer.h',
        'app/webrtc/test/mockpeerconnectionobservers.h',
        'app/webrtc/test/testsdpstrings.h',
        'app/webrtc/videotrack_unittest.cc',
        'app/webrtc/webrtcsdp_unittest.cc',
        'app/webrtc/webrtcsession_unittest.cc',
      ],
    },  # target libjingle_peerconnection_unittest
  ],
  'conditions': [
    ['OS=="linux"', {
      'targets': [
        {
          'target_name': 'libjingle_peerconnection_test_jar',
          'type': 'none',
          'actions': [
            {
              'variables': {
                'java_src_dir': 'app/webrtc/javatests/src',
                'java_files': [
                  'app/webrtc/javatests/src/org/webrtc/PeerConnectionTest.java',
                ],
              },
              'action_name': 'create_jar',
              'inputs': [
                'build/build_jar.sh',
                '<@(java_files)',
                '<(PRODUCT_DIR)/libjingle_peerconnection.jar',
                '<(DEPTH)/third_party/junit/junit-4.11.jar',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/libjingle_peerconnection_test.jar',
              ],
              'action': [
                'build/build_jar.sh', '/usr', '<@(_outputs)',
                '<(INTERMEDIATE_DIR)',
                '<(java_src_dir):<(PRODUCT_DIR)/libjingle_peerconnection.jar:<(DEPTH)/third_party/junit/junit-4.11.jar',
                '<@(java_files)'
              ],
            },
          ],
        },
        {
          'target_name': 'libjingle_peerconnection_java_unittest',
          'type': 'none',
          'actions': [
            {
              'action_name': 'copy libjingle_peerconnection_java_unittest',
              'inputs': [
                'app/webrtc/javatests/libjingle_peerconnection_java_unittest.sh',
                '<(PRODUCT_DIR)/libjingle_peerconnection_test_jar',
                '<(DEPTH)/third_party/junit/junit-4.11.jar',
              ],
              'outputs': [
                '<(PRODUCT_DIR)/libjingle_peerconnection_java_unittest',
              ],
              'action': [
                'bash', '-c',
                'rm -f <(PRODUCT_DIR)/libjingle_peerconnection_java_unittest && '
                'sed -e "s@GYP_JAVA_HOME@<(java_home)@" '
                '< app/webrtc/javatests/libjingle_peerconnection_java_unittest.sh '
                '> <(PRODUCT_DIR)/libjingle_peerconnection_java_unittest && '
                'cp <(DEPTH)/third_party/junit/junit-4.11.jar <(PRODUCT_DIR) && '
                'chmod u+x <(PRODUCT_DIR)/libjingle_peerconnection_java_unittest'
              ],
            },
          ],
        },
      ],
    }],
    ['libjingle_objc == 1', {
      'targets': [
        {
          'variables': {
            'infoplist_file': './app/webrtc/objctests/Info.plist',
          },
          'target_name': 'libjingle_peerconnection_objc_test',
          'type': 'executable',
          'mac_bundle': 1,
          'mac_bundle_resources': [
            '<(infoplist_file)',
          ],
          # The plist is listed above so that it appears in XCode's file list,
          # but we don't actually want to bundle it.
          'mac_bundle_resources!': [
            '<(infoplist_file)',
          ],
          'xcode_settings': {
            'INFOPLIST_FILE': '<(infoplist_file)',
          },
          'dependencies': [
            'gunit',
            'libjingle.gyp:libjingle_peerconnection_objc',
          ],
          'FRAMEWORK_SEARCH_PATHS': [
            '$(inherited)',
            '$(SDKROOT)/Developer/Library/Frameworks',
            '$(DEVELOPER_LIBRARY_DIR)/Frameworks',
          ],
          'sources': [
            'app/webrtc/objctests/RTCPeerConnectionSyncObserver.h',
            'app/webrtc/objctests/RTCPeerConnectionSyncObserver.m',
            'app/webrtc/objctests/RTCPeerConnectionTest.mm',
            'app/webrtc/objctests/RTCSessionDescriptionSyncObserver.h',
            'app/webrtc/objctests/RTCSessionDescriptionSyncObserver.m',
          ],
          'include_dirs': [
            '<(DEPTH)/talk/app/webrtc/objc/public',
          ],
          'conditions': [
            [ 'OS=="mac"', {
              'sources': [
                'app/webrtc/objctests/mac/main.mm',
              ],
              'xcode_settings': {
                'CLANG_ENABLE_OBJC_ARC': 'YES',
                'CLANG_WARN_OBJC_MISSING_PROPERTY_SYNTHESIS': 'NO',
                'CLANG_LINK_OBJC_RUNTIME': 'YES',
                # build/common.gypi disables ARC by default for back-compat
                # reasons with OSX 10.6.   Enabling OBJC runtime and clearing
                # LDPLUSPLUS and CC re-enables it.  Setting deployment target to
                # 10.7 as there are no back-compat issues with ARC.
                # https://code.google.com/p/chromium/issues/detail?id=156530
                'CC': '',
                'LDPLUSPLUS': '',
                'macosx_deployment_target': '10.7',
              },
            }],
          ],
        },
      ],
    }],
  ],
}
