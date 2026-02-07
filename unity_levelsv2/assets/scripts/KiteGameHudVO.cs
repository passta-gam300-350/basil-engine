using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using System;
using System.Collections.Generic;
using BasilEngine.SceneManagement;


public class KiteGameHudVO : Behavior
{
    struct HudLine
    {
        public HudComponent hud;
        public float duration;

        public HudLine(HudComponent h, float d)
        {
            hud = h;
            duration = d;
        }
    }

    Dictionary<string, HudLine[]> sequences =
        new Dictionary<string, HudLine[]>();

    HudLine[] currentSequence;
    int index = 0;
    float timer = 0f;
    bool playing = false;

    public void Init()
    {
        // INTRO
        sequences.Add("Intro", new HudLine[]
        {
            new HudLine(FindHUD("VOtext1_1"), 2.0f),
            new HudLine(FindHUD("VOtext1_2"), 2.0f),
            new HudLine(FindHUD("VOtext1_3"), 2.0f)
        });

        // FATHER WIN 1
        sequences.Add("Father1", new HudLine[]
        {
            new HudLine(FindHUD("VOtext2_1"), 0.8f),
            new HudLine(FindHUD("VOtext2_2"), 2.0f)
        });

        // FATHER WIN 2
        sequences.Add("Father2", new HudLine[]
        {
            new HudLine(FindHUD("VOtext3_1"), 2.5f)
        });

        // FATHER WIN 3
        sequences.Add("Father3", new HudLine[]
        {
            new HudLine(FindHUD("VOtext4_1"), 1.0f),
            new HudLine(FindHUD("VOtext4_2"), 2.5f)
        });

        // ENDING
        sequences.Add("Ending", new HudLine[]
        {
            new HudLine(FindHUD("VOtext5_1"), 3.0f),
            new HudLine(FindHUD("VOtext5_2"), 3.0f)
        });

        HideAll();
    }

    HudComponent FindHUD(string name)
    {
        GameObject obj = GameObject.Find(name);
        if (obj != null)
            return obj.transform.GetComponent<HudComponent>();

        Logger.Warn(name + " HUD not found");
        return null;
    }

    public void Play(string name)
    {
        if (!sequences.ContainsKey(name))
        {
            Logger.Warn("Sequence not found: " + name);
            return;
        }

        HideAll();

        currentSequence = sequences[name];
        index = 0;
        playing = true;

        if (currentSequence.Length > 0)
        {
            Show(index);
            timer = currentSequence[index].duration;
        }
    }

    public void Update()
    {
        if (!playing || currentSequence == null)
            return;

        timer -= Time.deltaTime;

        if (timer <= 0f)
        {
            index++;

            if (index >= currentSequence.Length)
            {
                HideAll();
                playing = false;
                return;
            }

            Show(index);
            timer = currentSequence[index].duration;
        }
    }

    void Show(int i)
    {
        for (int j = 0; j < currentSequence.Length; j++)
        {
            if (currentSequence[j].hud != null)
                currentSequence[j].hud.Visible = (j == i);
        }
    }

    void HideAll()
    {
        foreach (var pair in sequences)
        {
            HudLine[] list = pair.Value;

            for (int i = 0; i < list.Length; i++)
            {
                if (list[i].hud != null)
                    list[i].hud.Visible = false;
            }
        }
    }

}
