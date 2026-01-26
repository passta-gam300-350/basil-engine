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
    private GameObject[] footsteps;
    private System.Random random = new System.Random();
    private float footstepTimer = 0f;
    public float footstepInterval = 0.5f; // Time between footsteps in seconds

    public float mouseSensitivity = 0.075f;
    public float moveSpeed = 4f;

    public bool disabled = false;

    private Vector2 lastMouse;
    private float pitch = 0f; // X rotation
    private float yaw = 0f;   // Y rotation

    public bool onMopEnabled = false;

    private GameObject mopVisual;

    public bool canCollectTrash = true;

    public void Init()
    {
        // throw new Exception("This is a test for init");
        PlayerHead = GameObject.Find("Camera");
        rb = gameObject.transform.GetComponent<Rigidbody>();

        footsteps = new GameObject[9];
        for (int i = 0; i < footsteps.Length; i++)
        {
            string objectName = "footstep_" + (i + 1);
            footsteps[i] = GameObject.Find(objectName);
            if (footsteps[i] == null)
            {
                Logger.Warn("Cannot find game object: " + objectName);
            }
        }

        // Find the mop (Cube)
        mopVisual = GameObject.Find("Cube2");
        if (mopVisual != null)
        {
            mopVisual.visibility = false;
        }
        else
        {
            Logger.Warn("Cube (mop) not found!");
        }
    }

    public void Update()
    {
        if (disabled) return;

        //-------------------------------------------------------
        // 2. Movement (WASD) - FPS style on ground plane
        //-------------------------------------------------------
        Vector3 forward = PlayerHead.transform.forward; forward.y = 0f;
        Vector3 right = PlayerHead.transform.right; right.y = 0f;

        Vector3 vel = Vector3.Zero;

        if (Input.GetKey(KeyCode.W)) vel += forward;
        if (Input.GetKey(KeyCode.S)) vel -= forward;
        // Right vector appears mirrored in engine space, so flip strafing directions
        if (Input.GetKey(KeyCode.A)) vel += right; // move left
        if (Input.GetKey(KeyCode.D)) vel -= right; // move right

        bool previousMopState = onMopEnabled;

        if (Input.GetKey(KeyCode.KEY_1))
        {
            onMopEnabled = false;
            canCollectTrash = true;
        }
        else if (Input.GetKey(KeyCode.KEY_2))
        {
            onMopEnabled = true;
            canCollectTrash = false;
        }

        // Update mop when state changes
        if (onMopEnabled != previousMopState && mopVisual != null)
        {
            mopVisual.visibility = onMopEnabled;
            
            Logger.Log("Mop visual: " + (onMopEnabled ? "shown" : "hidden"));
        }

        bool isMoving = vel.MagnitudeSqr() > 0.001f;

        // Play footstep sounds at intervals while moving
        if (isMoving)
        {
            footstepTimer += Time.deltaTime;
            if (footstepTimer >= footstepInterval)
            {
                PlayFootstep();
                footstepTimer = 0f; // Reset timer
            }
        }
        else
        {
            // Reset timer when not moving
            footstepTimer = 0f;
        }

        if (isMoving)
            vel = vel.Normalize() * moveSpeed * Time.deltaTime; // Assuming ~60 FPS, so deltaTime ~0.016s

        rb.MovePosition(transform.position + vel);
    }

    private void PlayFootstep()
    {
        if (footsteps == null || footsteps.Length == 0)
            return;

        // Get a random footstep sound
        int randomIndex = random.Next(0, footsteps.Length);
        if (footsteps[randomIndex] != null)
        {
            Audio audio = footsteps[randomIndex].transform.GetComponent<Audio>();
            if (audio != null)
            {
                audio.Play();
            }
        }

    }

    public void FixedUpdate()
    {
    }

    public void OnCollisionEnter(GameObject other)
    {
        Logger.Log("Entered collision with " + other.NativeID);
    }

    public void Mop()
    {
        
    }
}
