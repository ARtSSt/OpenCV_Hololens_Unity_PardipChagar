//  
// Copyright (c) 2017 Vulcan, Inc. All rights reserved.  
// Licensed under the Apache 2.0 license. See LICENSE file in the project root for full license information.
//

using UnityEngine;
using UnityEngine.Networking;
using UnityEngine.UI;
using System.IO;
using System;
using System.Collections;

public class VideoPanel : MonoBehaviour
{
    public RawImage rawImage;

    public void SetResolution(int width, int height)
    {
        var texture = new Texture2D(width, height, TextureFormat.BGRA32, false);
        rawImage.texture = texture;
    }

    public void SetBytes(byte[] image)
    {

        var texture = rawImage.texture as Texture2D;
        texture.LoadRawTextureData(image); //TODO: Should be able to do this: texture.LoadRawTextureData(pointerToImage, 1280 * 720 * 4);
        texture.Apply();
    }

    public void SaveFrame(byte[] image, int numFrames)
    {
        if (numFrames < 60)
        {
            var texture = new Texture2D(896, 504, TextureFormat.BGRA32, false);
            //texture.LoadRawTextureData(image);
            //texture.Apply();
            //byte[] bytes = texture.EncodeToPNG();
            //string path = Path.Combine(Application.persistentDataPath, "HoloCapture" + numFrames + ".png");
            //FileStream stream = new FileStream(path, FileMode.OpenOrCreate, FileAccess.Write);
            //BinaryWriter writer = new BinaryWriter(stream);
            //for (int i = 0; i < bytes.Length; i++)
            //{
            //    writer.Write(bytes[i]);
            //}
            //writer.Close();
            //stream.Dispose();
        }
    }

    public void SendOutFrame (byte[] frame, int limit)
    {
        string data = Convert.ToBase64String(frame); 
        UnityWebRequest www = UnityWebRequest.Put("http://100.80.230.214/~pardipchagar/sunisshining/file.php", data);
        www.Send();
    }
}
