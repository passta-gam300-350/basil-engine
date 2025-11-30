using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using BasilEngine.SceneManagement;

public class KiteController : Behavior
{
    private Rigidbody rb;

    public float windSpeed = 30f;
    public float moveSpeed = 250f;

    public int totalKite = 4;



    public void Init()
    {

    }

    public void Update()
    {
        if (totalKite <= 0)
        {
            Scene.LoadSceneByIndex(0);
        }


        float time = Time.deltaTime;
        if (rb == null)
        {
            rb = GetComponent<Rigidbody>();
            if (rb == null) return;
            rb.UseGravity = false;
            rb.Drag = 0;


            return;
        }

        Vector3 movement = new Vector3();
        if (Input.GetKey(KeyCode.W))
        {
            movement.y = moveSpeed * time;
        }
        if (Input.GetKey(KeyCode.S))
        {
            movement.y = -moveSpeed * time;
        }
        
        if (Input.GetKey(KeyCode.D))
        {
            movement.x = -moveSpeed * time;
        }
        if (Input.GetKey(KeyCode.A))
        {
            movement.x = moveSpeed * time;
        }

        if (Input.GetKey(KeyCode.Q))
        {
            movement.z += moveSpeed * time;
        }
        if (Input.GetKey(KeyCode.E))
        {
            movement.z -= moveSpeed * time;
        }


        movement.x -= windSpeed * time;
        movement.y -= windSpeed * time;
        rb.velocity = movement;
    }

    public void FixedUpdate()
    {
    }
}