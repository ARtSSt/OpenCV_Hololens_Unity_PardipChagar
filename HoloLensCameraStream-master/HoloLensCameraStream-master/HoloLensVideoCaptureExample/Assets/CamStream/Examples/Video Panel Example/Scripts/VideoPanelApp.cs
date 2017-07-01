//  
// Copyright (c) 2017 Vulcan, Inc. All rights reserved.  
// Licensed under the Apache 2.0 license. See LICENSE file in the project root for full license information.
//

using UnityEngine;
using System.Runtime.InteropServices;
using System.IO;
using HoloLensCameraStream;
using HololensIO;

/// <summary>
/// This example gets the video frames at 30 fps and displays them on a Unity texture,
/// which is locked the User's gaze.
/// </summary>
public class VideoPanelApp : MonoBehaviour
{

    static FileIO _file;
    byte[] pixelOutBuffer;

    GCHandle pixelInBufferPtr;
    GCHandle pixelOutBufferPtr;
    bool bufferCreated = false;
    static int framesSaved;
    bool runOnce;

    [DllImport("HoloPlugin", EntryPoint = "DetectContours")]
    public static extern int DetectContours(byte[] inPixels, int w, int h, byte[] outPixels);

    byte[] _latestImageBytes;
    HoloLensCameraStream.Resolution _resolution;

    //"Injected" objects.
    VideoPanel _videoPanelUI;
    VideoCapture _videoCapture;

    void Start()
    {
        framesSaved = 0;
        runOnce = true;
        //Call this in Start() to ensure that the CameraStreamHelper is already "Awake".
        CameraStreamHelper.Instance.GetVideoCaptureAsync(OnVideoCaptureCreated);
        //You could also do this "shortcut":
        //CameraStreamManager.Instance.GetVideoCaptureAsync(v => videoCapture = v);
        _videoPanelUI = GameObject.FindObjectOfType<VideoPanel>();
    }

    private void OnDestroy()
    {
        if (_videoCapture != null)
        {
            _videoCapture.FrameSampleAcquired -= OnFrameSampleAcquired;
            _videoCapture.Dispose();
        }
    }

    void OnVideoCaptureCreated(VideoCapture videoCapture)
    {
        if (videoCapture == null)
        {
            Debug.LogError("Did not find a video capture object. You may not be using the HoloLens.");
            return;
        }
        
        this._videoCapture = videoCapture;

        _resolution = CameraStreamHelper.Instance.GetLowestResolution();
        float frameRate = CameraStreamHelper.Instance.GetHighestFrameRate(_resolution);
        videoCapture.FrameSampleAcquired += OnFrameSampleAcquired;

        CameraParameters cameraParams = new CameraParameters();
        cameraParams.cameraResolutionHeight = _resolution.height;
        cameraParams.cameraResolutionWidth = _resolution.width;
        cameraParams.frameRate = Mathf.RoundToInt(frameRate);
        cameraParams.pixelFormat = CapturePixelFormat.BGRA32;

        UnityEngine.WSA.Application.InvokeOnAppThread(() => {
            _videoPanelUI.SetResolution(_resolution.width, _resolution.height);
        }, false);

        videoCapture.StartVideoModeAsync(cameraParams, OnVideoModeStarted);
    }

    void OnVideoModeStarted(VideoCaptureResult result)
    {
        if (result.success == false)
        {
            Debug.LogWarning("Could not start video mode.");
            return;
        }

        Debug.Log("Video capture started.");
    }

    void OnFrameSampleAcquired(VideoCaptureSample sample)
    {
        //When copying the bytes out of the buffer, you must supply a byte[] that is appropriately sized.
        //You can reuse this byte[] until you need to resize it (for whatever reason).
        if (_latestImageBytes == null || _latestImageBytes.Length < sample.dataLength 
            || pixelOutBuffer == null || pixelOutBuffer.Length < sample.dataLength)
        {
            _latestImageBytes = new byte[sample.dataLength];
            pixelOutBuffer = new byte[sample.dataLength];
        }

        sample.CopyRawImageDataIntoBuffer(_latestImageBytes);
        sample.Dispose();

        //framesSaved++;
        //if (framesSaved < 29)
        //{
        //    UnityEngine.WSA.Application.InvokeOnAppThread(() =>
        //    {
        //        _videoPanelUI.SaveFrame(_latestImageBytes, framesSaved);
        //    }, true);
        //}

        if (bufferCreated)
        {
            pixelInBufferPtr.Free();
            pixelOutBufferPtr.Free();
            bufferCreated = false;
        }

        pixelInBufferPtr = GCHandle.Alloc(_latestImageBytes, GCHandleType.Pinned);
        pixelOutBufferPtr = GCHandle.Alloc(pixelOutBuffer, GCHandleType.Pinned);

        DetectContours(_latestImageBytes, 896, 504, pixelOutBuffer);
        bufferCreated = true;

        if (bufferCreated)
        {
            pixelInBufferPtr.Free();
            pixelOutBufferPtr.Free();
            bufferCreated = false;
        }

        //This is where we actually use the image data
        UnityEngine.WSA.Application.InvokeOnAppThread(() =>
        {
            _videoPanelUI.SetBytes(pixelOutBuffer);
            if (runOnce)
            {
                _videoPanelUI.SendOutFrame(pixelOutBuffer, 0);
                runOnce = false;
            }
        }, false);

        
    }
}
