using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class PlayerController3D : Behavior
{
    private GameObject PlayerHead;
    private Camera cam;
    private Rigidbody rb;

    public float mouseSensitivity = 0.075f;
    public float moveSpeed = 0.7f;

    public bool disabled = false;

    private Vector2 lastMouse;
    private float pitch = 0f; // X rotation
    private float yaw = 0f;   // Y rotation

    





    public void Init()
    {
       // throw new Exception("This is a test for init");
       PlayerHead = GameObject.Find("Camera");
       rb = gameObject.transform.GetComponent<Rigidbody>();
    }

    public void Update()
    {
        if (disabled) return;
        
		//-------------------------------------------------------
		// 2. Movement (WASD) - FPS style on ground plane
		//-------------------------------------------------------
		Vector3 forward = PlayerHead.transform.forward; forward.y = 0f;
		Vector3 right = PlayerHead.transform.right;     right.y = 0f;

        Vector3 vel = Vector3.Zero;

		if (Input.GetKey(KeyCode.W)) vel += forward;
		if (Input.GetKey(KeyCode.S)) vel -= forward;
		// Right vector appears mirrored in engine space, so flip strafing directions
		if (Input.GetKey(KeyCode.A)) vel += right; // move left
		if (Input.GetKey(KeyCode.D)) vel -= right; // move right

        if (vel.MagnitudeSqr() > 0.001f)
            vel = vel.Normalize() * moveSpeed * Time.deltaTime; // Assuming ~60 FPS, so deltaTime ~0.016s

        rb.MovePosition(transform.position + vel);
    }

    public void FixedUpdate()
    {
    }

    public void OnCollisionEnter(GameObject other)
    {
        Logger.Log("Entered collision with " + other.NativeID);
    }
}
