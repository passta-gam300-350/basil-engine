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

    private GameObject[] mopSounds;
    private System.Random random = new System.Random();

    public void Init()
    {
        playerController = playerObject.transform.GetComponent<PlayerController3D>();
        initialScale = transform.scale;

        mopSounds = new GameObject[5];
        for (int i = 0; i < mopSounds.Length; i++)
        {
            string objectName = "mop_sound_" + (i + 1); // mop_sound_1, mop_sound_2, mop_sound_3
            mopSounds[i] = GameObject.Find(objectName);

            if (mopSounds[i] == null)
            {
                Logger.Warn("Cannot find game object: " + objectName);
            }
        }

        UpdatePuddleSize();
    }

    public void Update()
    {

        if (playerController.onMopEnabled && 
            Vector3.DistanceSqr(playerObject.transform.position, transform.position) <= wipeDist)
        {
            bool rHeldNow = playerController.wantsToMop;

            if (rHeldNow && !wasRHeld)
            {
                PlayMopSound();
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

    private void PlayMopSound()
    {
        if (mopSounds == null || mopSounds.Length == 0)
            return;

        int randomIndex = random.Next(0, mopSounds.Length);

        if (mopSounds[randomIndex] != null)
        {
            Audio audio = mopSounds[randomIndex].transform.GetComponent<Audio>();
            if (audio != null)
            {
                audio.Play();
            }
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