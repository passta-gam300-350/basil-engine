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
    public float waitsec = 3f;
    private bool passA;
    private Video video;

    public void Init()
    {
        video = transform.GetComponent<Video>();
        video.isPlaying = true;
    }

    public void Update()
    {
        if (video != null && !video.isPlaying)
        {
            waitsec -= Time.deltaTime;

        } else if (passA)
        {
            waitsec -= Time.deltaTime;
        }

        if (waitsec <= 2f && !passA)
        {
            passA = true;
            gameObject.transform.DeleteComponent<Video>();
            video = null;
        }


        if (waitsec <= 0f)
        {

            Scene.LoadSceneByIndex(2);

        }
    }

    public void FixedUpdate()
    {
    }
}
