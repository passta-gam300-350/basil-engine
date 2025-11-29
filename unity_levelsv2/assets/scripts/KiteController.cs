using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class KiteController : Behavior
{
    private Rigidbody rb;

    public float windSpeed = 0.8f;
    public float moveSpeed = 2f;



    public void Init()
    {

    }

    public void Update()
    {
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
            movement.y = moveSpeed;
        }
        if (Input.GetKey(KeyCode.S))
        {
            movement.y = -moveSpeed;
        }
        
        if (Input.GetKey(KeyCode.D))
        {
            movement.x = -moveSpeed;
        }
        if (Input.GetKey(KeyCode.A))
        {
            movement.x = moveSpeed;
        }

        if (Input.GetKey(KeyCode.Q))
        {
            movement.z += moveSpeed;
        }
        if (Input.GetKey(KeyCode.E))
        {
            movement.z -= moveSpeed;
        }


        movement.x -= windSpeed;
        rb.velocity = movement;
    }

    public void FixedUpdate()
    {
    }
}