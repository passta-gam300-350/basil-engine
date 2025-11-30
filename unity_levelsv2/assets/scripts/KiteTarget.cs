using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class KiteTarget : Behavior
{
    private Rigidbody rb;

    public float windSpeed = 20f;
    public float collectDist = 5.5f;
    private GameObject player;
    private KiteController kiteController;
    private GameObject[] kiteHit;
    private System.Random random = new System.Random();

    public void Init()
    {
        kiteHit = new GameObject[4];
        for (int i = 0; i < 4; i++) 
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

        Vector3 movement = new Vector3();


        movement.x -= windSpeed * Time.deltaTime;
        movement.y -= windSpeed * Time.deltaTime;
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