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
            kiteHit[i] = GameObject.Find("kite_hit" + i);
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

            GameObject.Destroy(gameObject);
            kiteController.totalKite -= 1;
            int randomIndex = random.Next(0, kiteHit.Length);
            kiteHit[randomIndex].transform.GetComponent<Audio>().Play();
        }   
        Logger.Log(d);
    }

    public void FixedUpdate()
    {
    }
}