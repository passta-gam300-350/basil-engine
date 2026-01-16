using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;



public class DirtyPuddle : Behavior
{
    public PlayerController3D playerController;
    public GameObject playerObject;
    public float wipeDist = 0.02f;
    public void Init()
    {
        playerController = playerObject.transform.GetComponent<PlayerController3D>();
    }
    public void Update()
    {
        if (playerController.onMopEnabled &&
            Vector3.DistanceSqr(playerObject.transform.position, transform.position) <= wipeDist)
        {
            if (Input.GetKey(KeyCode.R))
            {
                Logger.Log("Puddle cleaned!");
                GameObject.Destroy(gameObject);

            }
        }
    }

    public void FixedUpdate()
    {

    }


}