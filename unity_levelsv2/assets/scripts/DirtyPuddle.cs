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
    private int wipeCount = 0;
     private bool wasRHeld = false;
     private Vector3 initialScale;

    public void Init()
    {
        playerController = playerObject.transform.GetComponent<PlayerController3D>();
        initialScale = transform.scale;
        UpdatePuddleSize();
    }

    public void Update()
    {
        if (playerController.onMopEnabled &&
            Vector3.DistanceSqr(playerObject.transform.position, transform.position) <= wipeDist)
        {

            bool rHeldNow = Input.GetKey(KeyCode.R);

            if (rHeldNow && !wasRHeld)
            {
                wipeCount++;
                Logger.Log("Wipe count: " + wipeCount);

                if (wipeCount == 3)
                {
                    Logger.Log("Puddle cleaned!");
                    GameObject.Destroy(gameObject);
                    return;
                }

                UpdatePuddleSize();
            }

            wasRHeld = rHeldNow;
        }else{
            wasRHeld = false;
        }

    }

    private void UpdatePuddleSize()
    {
        // 0 = original, 1 = smaller, 2 = smaller, 3 = destroy
        if (wipeCount <= 0)
        {
            transform.scale = initialScale;
        }
        else if (wipeCount == 1)
        {
            transform.scale = initialScale * 0.66f;
        }
        else // wipeCount == 2
        {
            transform.scale = initialScale * 0.33f;
        }
    }

    public void FixedUpdate()
    {

    }


}