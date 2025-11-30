using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;
using System.Runtime.InteropServices;


public class Spool : Behavior
{

    private Audio audio;
    public float rotationSpeed = 90.0f; // Degrees per second
    public float maxRotation = 30.0f; // Maximum rotation angle in degrees
    public float moveSpeed = .1f; // Units per second for vertical movement
    public float maxPosition = .02f; // Maximum vertical offset from initial position
    
    private float targetRotation = 0.0f;
    private float targetY = 0.0f;
    private float initialY = 0.0f;
    private bool wasPlaying = false;

    public void Init()
    {
        audio = transform.GetComponent<Audio>();
        initialY = gameObject.transform.position.y;
        targetY = initialY;
    }
    public void Update()
    {
        Vector3 currentRotation = gameObject.transform.rotation;
        float currentZ = currentRotation.z;
        bool isKeyPressed = false;
        
        // Determine target rotation based on input
        if (Input.GetKey(KeyCode.A))
        {
            targetRotation = -maxRotation;
            isKeyPressed = true;
        }
        else if (Input.GetKey(KeyCode.D))
        {
            targetRotation = maxRotation;
            isKeyPressed = true;
        }
        else
        {
            targetRotation = 0.0f;
        }
        
        // Check for W/S keys for movement (also triggers audio)
        if (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.S))
        {
            isKeyPressed = true;
        }
        
        // Play audio while any key (A, D, W, or S) is pressed
        if (isKeyPressed && !wasPlaying)
        {
            audio.Play();
            wasPlaying = true;
        }
        else if (!isKeyPressed && wasPlaying)
        {
            audio.Stop();
            wasPlaying = false;
        }
        
        // Smoothly rotate towards target
        float rotationStep = rotationSpeed * Time.deltaTime;
        if (currentZ < targetRotation)
        {
            currentZ = System.Math.Min(currentZ + rotationStep, targetRotation);
        }
        else if (currentZ > targetRotation)
        {
            currentZ = System.Math.Max(currentZ - rotationStep, targetRotation);
        }
        
        // Preserve x and y rotation, only update z
        gameObject.transform.rotation = new Vector3(currentRotation.x, currentRotation.y, currentZ);
        
        // Handle vertical movement with W/S keys
        Vector3 currentPosition = gameObject.transform.position;
        float currentY = currentPosition.y;
        
        // Determine target Y position based on input
        if (Input.GetKey(KeyCode.W))
        {
            targetY = initialY + maxPosition;
        }
        else if (Input.GetKey(KeyCode.S))
        {
            targetY = initialY - maxPosition;
        }
        else
        {
            targetY = initialY;
        }
        
        // Smoothly move towards target Y position
        float moveStep = moveSpeed * Time.deltaTime;
        if (currentY < targetY)
        {
            currentY = System.Math.Min(currentY + moveStep, targetY);
        }
        else if (currentY > targetY)
        {
            currentY = System.Math.Max(currentY - moveStep, targetY);
        }
        
        // Preserve x and z position, only update y
        gameObject.transform.position = new Vector3(currentPosition.x, currentY, currentPosition.z);
    }

    public void FixedUpdate()
    {

    }


}