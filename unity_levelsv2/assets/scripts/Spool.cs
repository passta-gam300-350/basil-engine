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
    
    private float targetRotation = 0.0f;
    private bool wasPlaying = false;

    public void Init()
    {
        audio = transform.GetComponent<Audio>();
    }
    public void Update()
    {
        Vector3 currentRotation = gameObject.transform.rotation;
        float currentZ = currentRotation.z;
        bool isKeyPressed = false;
        
        // Determine target rotation based on input
        if (Input.GetKey(KeyCode.A))
        {
            targetRotation = maxRotation;
            isKeyPressed = true;
        }
        else if (Input.GetKey(KeyCode.D))
        {
            targetRotation = -maxRotation;
            isKeyPressed = true;
        }
        else
        {
            targetRotation = 0.0f;
        }
        
        // Play audio while key is pressed
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
    }

    public void FixedUpdate()
    {

    }


}