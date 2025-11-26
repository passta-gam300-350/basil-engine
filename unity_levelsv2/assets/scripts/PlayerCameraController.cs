using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class PlayerCameraPlacementController : Behavior
{

    public float mouseSensitivity = 0.075f;

    private Vector2 lastMouse = new Vector2(0f, 0f);
    private float pitch = 0f; // X rotation
    private float yaw = 0f;   // Y rotation





    public void Init()
    {
        lastMouse = Input.mousePosition;
    }

    public void Update()
    {
        // Lazy init camera reference and align our yaw/pitch with current transform once
        
        //-------------------------------------------------------
        // 1. Mouse Look (delta from last frame)
        //-------------------------------------------------------
        Vector2 mouse = Input.mousePosition;
        Vector2 delta = mouse - lastMouse;
        lastMouse = mouse;

        // Invert yaw so mouse left rotates left (match engine handedness)
        yaw -= delta.x * mouseSensitivity;
        // Invert pitch direction so moving mouse up looks up
        pitch += delta.y * mouseSensitivity;

        pitch = Mathf.Clamp(pitch, -89f, 89f);

        // Apply camera rotation
        transform.rotation = new Vector3(pitch, yaw, 0f);
        // (Optional) add debug logging here if needed

        
    }

    public void FixedUpdate()
    {
    }

    public void OnCollisionEnter(GameObject other)
    {
        Logger.Log("Entered collision with " + other.NativeID);
    }
}
