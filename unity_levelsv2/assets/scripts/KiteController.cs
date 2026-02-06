using System.Linq.Expressions;
using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;
using BasilEngine.SceneManagement;

public class KiteController : Behavior
{
    private Rigidbody rb;

    private bool canMove = false; // player cannot move at start
    private bool introStarted = false;
    private Audio introAudio;
    private float introTimer = 0f; // counts down VO duration
    public float introDuration = 6.0f; // VO length

    public float windSpeed = 30f;
    public float moveSpeed = 250f;

    public int totalKite = 4;

    private Spool spool;

    private Audio fatherWin1;
    private Audio fatherWin2;
    private Audio fatherWin3;
    private Audio fatherWinCombine;

    private int kitesHitCount = 0;
    private bool waitingForFinalVO = false;
    private float finalVOTimer = 0f;
    public float finalVODuration = 6.5f;


    public void Init()
    {
        rb = GetComponent<Rigidbody>();
        if (rb != null)
        {
            rb.UseGravity = false;
            rb.Drag = 0;
        }

        GameObject voObj = GameObject.Find("KiteIntroVO1");
        if (voObj != null)
        {
            introAudio = voObj.transform.GetComponent<Audio>();
            if (introAudio == null)
            {
                Logger.Warn("KiteIntroVO Audio component missing!");
                canMove = true; // allow movement if VO missing
            }
        }
        else
        {
            Logger.Warn("KiteIntroVO object not found!");
            canMove = true; // allow movement if VO missing
        }

        GameObject spoolObj = GameObject.Find("spool");
        if (spoolObj != null)
        {
            spool = spoolObj.transform.GetComponent<Spool>();
        }
        else
        {
            Logger.Warn("Spool object not found!");
        }

        GameObject vo1 = GameObject.Find("Father_Win1");

        if (vo1 != null)
            fatherWin1 = vo1.transform.GetComponent<Audio>();
        else
            Logger.Warn("Father_Win1 not found");

        GameObject vo2 = GameObject.Find("Father_Win2");

        if (vo2 != null)
            fatherWin2 = vo2.transform.GetComponent<Audio>();
        else
            Logger.Warn("Father_Win2 not found");

        GameObject vo3 = GameObject.Find("Father_Win3");

        if (vo3 != null)
            fatherWin3 = vo3.transform.GetComponent<Audio>();
        else
            Logger.Warn("Father_Win3 not found");

        GameObject vo4 = GameObject.Find("Father_Win_combine");

        if (vo4 != null)
            fatherWinCombine = vo4.transform.GetComponent<Audio>();
        else
            Logger.Warn("Father_Win_combine not found");

    }

    public void Update()
    {
        if (!introStarted && introAudio != null)
        {
            introAudio.Play();
            introStarted = true;
            canMove = false; // ensure player cannot move while VO plays
            introTimer = introDuration; // set timer to clip length in seconds
            Logger.Log("Playing intro VO for " + introTimer + " seconds...");

            if (spool != null)
                spool.canControlSpool = false; // lock spool
        }

        // countdown the timer each frame
        if (introStarted && !canMove)
        {
            introTimer -= Time.deltaTime;
            if (introTimer <= 0f)
            {
                canMove = true; // enable movement
                Logger.Log("Intro VO finished, kite movement enabled!");

                if (spool != null)
                    spool.canControlSpool = true; // unlock spool
            }
        }

        if (waitingForFinalVO)
        {
            finalVOTimer -= Time.deltaTime;

            if (finalVOTimer <= 0f)
            {
                Logger.Log("Final Father VO finished, changing scene");
                Scene.LoadSceneByIndex(5);
            }

            return; // freeze everything while VO finishes
        }

        if (!canMove) return;

        float time = Time.deltaTime;
        if (rb == null)
        {

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

    public void OnKiteCollected()
    {
        kitesHitCount++;

        if (kitesHitCount == 1 && fatherWin1 != null)
        {
            fatherWin1.Play();
        }
        else if (kitesHitCount == 2 && fatherWin2 != null)
        {
            fatherWin2.Play();
        }
        else if (kitesHitCount == 3 && fatherWin3 != null)
        {
            fatherWin3.Play();
        }
        else if (kitesHitCount == 4 && fatherWinCombine != null)
        {
            fatherWinCombine.Play();
            waitingForFinalVO = true;
            finalVOTimer = finalVODuration;
        }
    }

    public void FixedUpdate()
    {
    }
}