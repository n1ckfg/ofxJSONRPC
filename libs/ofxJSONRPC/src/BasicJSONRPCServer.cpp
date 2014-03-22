// =============================================================================
//
// Copyright (c) 2014 Christopher Baker <http://christopherbaker.net>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// =============================================================================


#include "ofx/JSONRPC/BasicJSONRPCServer.h"


namespace ofx {
namespace JSONRPC {



BasicJSONRPCServer::BasicJSONRPCServer(const BasicJSONRPCServer::Settings& settings):
    ofx::HTTP::BasicServer(settings),
    _postRoute(ofx::HTTP::PostRoute::makeShared(settings)),
    _webSocketRoute(ofx::HTTP::WebSocketRoute::makeShared(settings))
{
    addRoute(_postRoute); // #2 for test
    addRoute(_webSocketRoute); // #1 for test

    _postRoute->registerPostEvents(this);
    _webSocketRoute->registerWebSocketEvents(this);
}


BasicJSONRPCServer::~BasicJSONRPCServer()
{
    _webSocketRoute->unregisterWebSocketEvents(this);
    _postRoute->unregisterPostEvents(this);

    removeRoute(_webSocketRoute);
    removeRoute(_postRoute);
}


ofx::HTTP::PostRoute::SharedPtr BasicJSONRPCServer::getPostRoute()
{
    return _postRoute;
}


ofx::HTTP::WebSocketRoute::SharedPtr BasicJSONRPCServer::getWebSocketRoute()
{
    return _webSocketRoute;
}


bool BasicJSONRPCServer::onWebSocketOpenEvent(ofx::HTTP::WebSocketEventArgs& evt)
{
    return false;  // We did not attend to this event, so pass it along.
}


bool BasicJSONRPCServer::onWebSocketCloseEvent(ofx::HTTP::WebSocketEventArgs& evt)
{
    return false;  // We did not attend to this event, so pass it along.
}


bool BasicJSONRPCServer::onWebSocketFrameReceivedEvent(ofx::HTTP::WebSocketFrameEventArgs& evt)
{
    Json::Value json;
    Json::Reader reader;

    if (reader.parse(evt.getFrameRef().getText(), json))
    {
        try
        {
            JSONRPC::Response response = processCall(&evt.getConnectionRef(),
                                                     JSONRPC::Request::fromJSON(json));

            if (response.hasID())
            {
                evt.getConnectionRef().sendFrame(response.toString());
            }
        }
        catch (Poco::Exception& exc)
        {
            JSONRPC::Response response(Json::Value::null, // null value is required when parse exceptions
                                       JSONRPC::Error::METHOD_NOT_FOUND);
            evt.getConnectionRef().sendFrame(response.toString());
        }

        return true;  // We attended to the event, so consume it.
    }
    else
    {
        ofLogNotice("WebSocketMethodResponder::onWebSocketFrameReceivedEvent") << "Could not parse as JSON: " << reader.getFormattedErrorMessages();
        return false;  // We did not attend to this event, so pass it along.
    }
}


bool BasicJSONRPCServer::onWebSocketFrameSentEvent(ofx::HTTP::WebSocketFrameEventArgs& evt)
{
    return false;  // We did not attend to this event, so pass it along.
}


bool BasicJSONRPCServer::onWebSocketErrorEvent(ofx::HTTP::WebSocketEventArgs& evt)
{
    return false;  // We did not attend to this event, so pass it along.
}


bool BasicJSONRPCServer::onHTTPFormEvent(HTTP::PostFormEventArgs& args)
{
    ofLogNotice("ofApp::onHTTPFormEvent") << "";
    HTTP::Utils::dumpNameValueCollection(args.form, ofGetLogLevel());
    return false;  // We did not attend to this event, so pass it along.
}


bool BasicJSONRPCServer::onHTTPPostEvent(HTTP::PostEventArgs& args)
{
    Json::Value json;
    Json::Reader reader;

    if (reader.parse(args.data.getText(), json))
    {
        try
        {
            JSONRPC::Response response = processCall(0, // TODO: pass session data
                                                     JSONRPC::Request::fromJSON(json));

            if (response.hasID())
            {
                std::string buffer = response.toString();
                args.response.sendBuffer(buffer.c_str(), buffer.length());
            }
        }
        catch (Poco::Exception& exc)
        {
            JSONRPC::Response response(Json::Value::null, // null value is required when parse exceptions
                                       JSONRPC::Error::METHOD_NOT_FOUND);
            std::string buffer = response.toString();
            args.response.sendBuffer(buffer.c_str(), buffer.length());
        }

        return true;  // We attended to the event, so consume it.
    }
    else
    {
        ofLogNotice("WebSocketMethodResponder::onWebSocketFrameReceivedEvent") << "Could not parse as JSON: " << reader.getFormattedErrorMessages();

        return false;  // We did not attend to this event, so pass it along.
    }
}


bool BasicJSONRPCServer::onHTTPUploadEvent(HTTP::PostUploadEventArgs& args)
{
    std::string stateString = "";

    switch (args.getState())
    {
        case HTTP::PostUploadEventArgs::UPLOAD_STARTING:
            stateString = "STARTING";
            break;
        case HTTP::PostUploadEventArgs::UPLOAD_PROGRESS:
            stateString = "PROGRESS";
            break;
        case HTTP::PostUploadEventArgs::UPLOAD_FINISHED:
            stateString = "FINISHED";
            break;
    }

    ofLogNotice("ofApp::onHTTPUploadEvent") << "";
    cout << "         state: " << stateString << endl;
    cout << " formFieldName: " << args.getFormFieldName() << endl;
    cout << "orig. filename: " << args.getOriginalFilename() << endl;
    cout << "      filename: " << args.getFilename() << endl;
    cout << "      fileType: " << args.getFileType().toString() << endl;
    cout << "# bytes xfer'd: " << args.getNumBytesTransferred() << endl;

    return false;
}


} } // namespace ofx::JSONRPC
