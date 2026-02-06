using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using BasilEngine.SceneManagement;

public class FanRotate : Behavior
{
    public float spinSpeed = 180f; // degrees per second

    public void Update()
    {
        Vector3 currentRotation = transform.rotation;

        float newY = currentRotation.y + spinSpeed * Time.deltaTime;

        transform.rotation = new Vector3(
            currentRotation.x,
            newY,
            currentRotation.z
        );
    }
}
