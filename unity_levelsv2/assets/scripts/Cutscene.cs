using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using System.Numerics;
using BasilEngine.SceneManagement;
using static System.Runtime.CompilerServices.RuntimeHelpers;

public class Cutscene : Behavior
{
    private Video video;

    public void Init()
    {
        video = transform.GetComponent<Video>();
        video.isPlaying = true;
    }

    public void Update()
    {
        if (!video.isPlaying)
            Scene.LoadSceneByIndex(2);
    }

    public void FixedUpdate()
    {
    }
}
