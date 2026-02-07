using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Debug;
using BasilEngine.Mathematics;
using System;

public class TrashBag : Behavior
{
    public static TrashBag instance;

    private GameObject ThrashCollect;

    public int trashInHand = 0;
    public int maxTrashInHand = 5;
    public bool bagIsFull = false;

    private GameObject player;
    private PlayerController3D controller;

    public GameObject maxTrashAudioObject;
    private Audio maxTrashAudio;

    public void Init()
    {
        instance = this;

        ThrashCollect = GameObject.Find("ThrashCollect");

        if (ThrashCollect == null)
            Logger.Warn("ThrashCollect not found!");

        player = GameObject.Find("PlayerGroup");

        if (player != null)
        {
            controller = player.transform.GetComponent<PlayerController3D>();
        }

        maxTrashAudioObject = GameObject.Find("MengMaxTrashAudio"); // all levels should have
        if (maxTrashAudioObject != null)
        {
            maxTrashAudio = maxTrashAudioObject.transform.GetComponent<Audio>();
            if (maxTrashAudio == null)
                Logger.Warn("MaxTrashAudio object found but Audio component missing!");
        }
        else
        {
            Logger.Warn("MaxTrashAudio object not found!");
        }

        if (ThrashCollect != null)
            ThrashCollect.visibility = true;
    }

    public void Update()
    {
        if (Input.GetKey(KeyCode.Q))
            DropBag();
    }

    // =========================
    // PUBLIC API
    // =========================

    public void CollectTrash()
    {
        if (bagIsFull) return;

        trashInHand++;

        if (trashInHand >= maxTrashInHand)
        {
            trashInHand = maxTrashInHand;
            bagIsFull = true;

            if (maxTrashAudio != null)
                maxTrashAudio.Play();
        }

        UpdateBagSize();
        UpdatePlayerSpeed();
    }

    public void EmptyBag()
    {
        trashInHand = 0;
        bagIsFull = false;

        UpdateBagSize();
        UpdatePlayerSpeed();
    }

    public void SetVisible(bool visible)
    {
        if (ThrashCollect != null)
            ThrashCollect.visibility = visible;
    }

    public void DropBag()
    {
        if (trashInHand == 0) return;

        trashInHand = 0;
        bagIsFull = false;

        UpdateBagSize();
        UpdatePlayerSpeed();

        Logger.Log("Bag dropped");
    }

    // =========================
    // INTERNALS
    // =========================

    void UpdateBagSize()
    {
        if (ThrashCollect == null) return;

        if (trashInHand == 0)
        {
            ThrashCollect.transform.scale = new Vector3(0.002f, 0.002f, 0.002f);
            ThrashCollect.transform.position = new Vector3(0f, -0.230f, 0.3f);
        }
        else if (trashInHand <= 2)
        {
            ThrashCollect.transform.scale = new Vector3(0.0021f, 0.0021f, 0.0021f);
            ThrashCollect.transform.position = new Vector3(0f, -0.235f, 0.3f);
        }
        else if (trashInHand <= 4)
        {
            ThrashCollect.transform.scale = new Vector3(0.0023f, 0.0023f, 0.0023f);
            ThrashCollect.transform.position = new Vector3(0f, -0.24f, 0.3f);
        }
        else
        {
            ThrashCollect.transform.scale = new Vector3(0.0025f, 0.0025f, 0.0025f);
            ThrashCollect.transform.position = new Vector3(0f, -0.245f, 0.3f);
        }
    }

    void UpdatePlayerSpeed()
    {
        if (controller == null) return;

        float minMultiplier = 0.2f;
        float t = (float)trashInHand / maxTrashInHand;

        controller.speedMultiplier = 1f - (t * (1f - minMultiplier));
    }

}
