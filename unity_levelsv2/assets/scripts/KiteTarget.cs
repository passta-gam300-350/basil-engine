using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class KiteTarget : Behavior
{
    private Rigidbody rb;

    public float windSpeed = 1f;
    public float collectDist = 5.5f;
    private GameObject player;
    private KiteController kiteController;
    private GameObject[] kiteHit;
    private System.Random random = new System.Random();

    public float minX = -10f;
    public float maxX = 10f;
    public float minY = 0f;
    public float maxY = 10f;

    // persistent velocity for bouncing
    private Vector3 movement;

    public void Init()
    {
        
        // kites' initial velocity
        movement = new Vector3(-windSpeed, 0f, 0f);
        kiteHit = new GameObject[4];
        for (int i = 0; i < kiteHit.Length; i++) 
        {
            string objectName = "kite_hit" + (i + 1);
            kiteHit[i] = GameObject.Find(objectName);
            if (kiteHit[i] == null)
            {
                Logger.Warn("Cannot find game object: " + objectName);
            }
        }
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

        // move kite
        Vector3 pos = transform.position;
        pos += movement * Time.deltaTime;

        // Bounce on X axis
        if (pos.x <= minX)
        {
            pos.x = minX;
            movement.x = Math.Abs(movement.x);   // bounce right
        }
        else if (pos.x >= maxX)
        {
            pos.x = maxX;
            movement.x = -Math.Abs(movement.x);  // bounce left
        }

        // Bounce on Y axis
        if (pos.y <= minY)
        {
            pos.y = minY;
            movement.y = Math.Abs(movement.y);   // bounce up
        }
        else if (pos.y >= maxY)
        {
            pos.y = maxY;
            movement.y = -Math.Abs(movement.y);  // bounce down
        }

        // apply new pos and velocity
        transform.position = pos;
        rb.velocity = movement;




        float d = Vector3.DistanceSqr(player.transform.position, transform.position);
        if (d < collectDist)
        {
            // Collect the kite
            int randomIndex = random.Next(0, kiteHit.Length);
            if (kiteHit[randomIndex] != null)
            {
                kiteHit[randomIndex].transform.GetComponent<Audio>().Play();
            }
            GameObject.Destroy(gameObject);
            kiteController.totalKite -= 1;
        }   
        Logger.Log(d);
    }

    public void FixedUpdate()
    {
    }
}