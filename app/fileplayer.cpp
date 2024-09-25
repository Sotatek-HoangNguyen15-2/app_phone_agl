/*
 * Copyright (C) 2018,2019 Konsulko Group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fileplayer.h"
#include <string>
#include <cstring>
#include <iostream>
#include <mutex>

#undef DEBUG

static void gst_loop_thread(GMainLoop *loop)
{
	if(loop)
		g_main_loop_run(loop);
}

FilePlayer::FilePlayer(const std::string &path, const std::string &role) :
	m_path(path),
	m_role(role)
{
	std::string uri;

        // Initialize GStreamer
        gst_init(NULL, NULL);

	// Create elements we need
	m_playbin = gst_element_factory_make("playbin", "play");
	m_audio_sink = gst_element_factory_make("pipewiresink", NULL);
	if(!(m_playbin && m_audio_sink))
		goto error;
	gst_util_set_object_arg(G_OBJECT(m_audio_sink), "stream-properties", "p,media.role=Communication");

	// Set up bus callback
	m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_playbin));
	if(!m_bus)
		goto error;
	m_gst_loop = g_main_loop_new(NULL, FALSE);
	if(!m_gst_loop)
		goto error;
	gst_bus_add_watch(m_bus, gstreamer_bus_callback, this);

	// Start thread to run glib main loop for gstreamer bus
	m_gst_thread = std::thread(gst_loop_thread, m_gst_loop);

	m_valid = true;
	return;
error:
	gst_object_unref(m_playbin);
	m_playbin = nullptr;
	gst_object_unref(m_audio_sink);
	m_audio_sink = nullptr;
	gst_object_unref(m_bus);
	m_bus = nullptr;

	if(m_gst_loop) {
		g_main_loop_quit(m_gst_loop);
		m_gst_loop = nullptr;
	}
	return;
}

FilePlayer::~FilePlayer(void)
{
	gst_element_set_state(m_playbin, GST_STATE_NULL);
	gst_object_unref(m_playbin);
	gst_object_unref(m_audio_sink);
	gst_object_unref(m_bus);
}

void FilePlayer::play(bool loop)
{
	if(!m_valid || m_playing)
		return;

	m_playing = true;
	m_looping = loop;

	g_object_set(m_playbin, "audio-sink", m_audio_sink, NULL);
	std::string uri = "file://" + m_path;
	g_object_set(m_playbin, "uri", uri.c_str(), NULL);

	// Start playback
	gst_element_set_state(m_playbin, GST_STATE_PLAYING);

	return;
}

void FilePlayer::stop(void)
{
	if(!(m_valid && m_playing))
		return;

	// Stop playback
	gst_element_set_state(m_playbin, GST_STATE_PAUSED);
}

gboolean FilePlayer::gstreamer_bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
	if(!data)
		return TRUE;

	return static_cast<FilePlayer*>(data)->bus_callback(bus, msg);
}

gboolean FilePlayer::bus_callback(__attribute__((unused)) GstBus *bus, GstMessage *msg)
{
	switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
#ifdef DEBUG
		std::cerr << __FUNCTION__ << ": GST_MESSAGE_EOS" << std::endl;
#endif
		if(m_looping) {
			// restart playback if at end
			if (!gst_element_seek(m_playbin, 
					      1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
					      GST_SEEK_TYPE_SET, 0,
					      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
				std::cerr << "Seek failed!" << std::endl;
			}
		} else {
			// Move stream to paused state since we're done
			gst_element_set_state(m_playbin, GST_STATE_PAUSED);
		}
		break;
	case GST_MESSAGE_STATE_CHANGED: {
		if((GstElement*) GST_MESSAGE_SRC(msg) != m_playbin)
			break;

		GstState old_state, new_state;
		gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
#ifdef DEBUG
		std::cerr << __FUNCTION__ << ": GST_MESSAGE_STATE_CHANGE: " << (int) old_state << " to " << (int) new_state << std::endl;
#endif
		if(old_state == GST_STATE_PLAYING && new_state == GST_STATE_PAUSED) {
			// Seek back to beginning so any subsequent play starts there
			if (!gst_element_seek(m_playbin, 
					      1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
					      GST_SEEK_TYPE_SET, 0,
					      GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
				std::cerr << "Seek failed!" << std::endl;
			}

			m_playing = false;
		} else if(old_state == GST_STATE_READY && new_state == GST_STATE_NULL) {
			// clean up
			g_main_loop_quit(m_gst_loop);
		}
		break;
	}
        default:
		break;
	}
	return TRUE;
}
