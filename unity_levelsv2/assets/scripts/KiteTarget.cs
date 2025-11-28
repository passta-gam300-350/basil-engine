using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class KiteTarget : Behavior
{
    private Rigidbody rb;

    public float windSpeed = 15f;
    public float collectDist = 2f;
    private GameObject player;
    private KiteController kiteController;

    public void Init()
    {
        
    }

    public void Update()
    {
        if (player == null)
        {
            player = GameObject.Find("Player");
            if (player == null) return;
            kiteController = player.transform.GetComponent<KiteController>();

        }
        if (rb == null)
        {
            rb = GetComponent<Rigidbody>();
            if (rb == null) return;
            rb.UseGravity = false;
            rb.Drag = 0;


            return;
        }

        Vector3 movement = new Vector3();


        movement.x -= windSpeed * Time.deltaTime;
        rb.velocity = movement;
        float d = Vector3.DistanceSqr(player.transform.position, transform.position);
        if (d < collectDist)
        {
            // Collect the kite

            GameObject.Destroy(gameObject);
            kiteController.totalKite -= 1;

        }   
        Logger.Log(d);
    }

    public void FixedUpdate()
    {
    }
}