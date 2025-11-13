using BasilEngine.Components;
using BasilEngine.Debug;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Configuration;
using System.Text;
using System.Threading.Tasks;

namespace GameAssembly
{
    public class AudioPlayer : Behavior
    {
        private Audio audio;
        public bool hasPlayed = false;

        public void Init()
        {

        }
        public void Update()
        {
            if (audio == null)
            {
                audio = GetComponent<Audio>();
                if (audio == null)
                {
                    Logger.Log("Audio is NULL");
                }
            }

            if (audio != null && hasPlayed == false)
            {
                audio.Play();
                hasPlayed = true;
                Logger.Log("Audio has been playing");
            }
        }

        public void FixedUpdate()
        {

        }


    }
}
