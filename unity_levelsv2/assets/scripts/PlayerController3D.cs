using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class PlayerController3D : Behavior
{
    private Camera cam;
    private Rigidbody rb;

    public float mouseSensitivity = 0.075f;
    public float moveSpeed = 2.5f;

    private Vector2 lastMouse;
    private float pitch = 0f; // X rotation
    private float yaw = 0f;   // Y rotation





    public void Init()
    {
       
    }

    public void Update()
    {
        // Lazy init camera reference and align our yaw/pitch with current transform once
        if (cam == null)
        {
            cam = GetComponent<Camera>();
            if (cam == null)
            {
                Logger.Warn("Camera is NULL!");
                return;
            }

            lastMouse = Input.mousePosition;
            Vector3 currentRot = transform.rotation;
            pitch = currentRot.x;
            yaw = currentRot.y;
        }

        if (rb == null)
        {
            rb = GetComponent<Rigidbody>();
            if (rb == null)
            {
                Logger.Warn("Rigidbody is NULL!");
                return;
            }

        }
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

		//-------------------------------------------------------
		// 2. Movement (WASD) - FPS style on ground plane
		//-------------------------------------------------------
		Vector3 forward = transform.forward; forward.y = 0f;
		Vector3 right = transform.right;     right.y = 0f;

        Vector3 vel = Vector3.Zero;

		if (Input.GetKey(KeyCode.W)) vel += forward;
		if (Input.GetKey(KeyCode.S)) vel -= forward;
		// Right vector appears mirrored in engine space, so flip strafing directions
		if (Input.GetKey(KeyCode.A)) vel += right; // move left
		if (Input.GetKey(KeyCode.D)) vel -= right; // move right

        if (vel.MagnitudeSqr() > 0.001f)
            vel = vel.Normalize() * moveSpeed * 0.016f; // Assuming ~60 FPS, so deltaTime ~0.016s

        rb.MovePosition(transform.position + vel);
    }

    public void FixedUpdate()
    {
    }
}
