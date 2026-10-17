import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec=importlib.util.spec_from_file_location('feedback',Path(__file__).resolve().parents[1]/'tools/analyze_medievil2_feedback.py')
feedback=importlib.util.module_from_spec(spec);spec.loader.exec_module(feedback)


class FeedbackTest(unittest.TestCase):
    def test_old_and_new_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory); session=root/'session';session.mkdir()
            (session/'session.json').write_text(json.dumps(dict(build='test')))
            input_rows=[]
            for seq in range(6):
                values=[seq,65535 if seq!=2 else 49151,65535,1,128,128,128,128,0,128,128,128,128,0,1,0]
                if seq in (3,4): values[4]=255;values[6]=0
                if seq==4: values[1]&=~0x20
                input_rows.append('N '+','.join(map(str,values)))
            (session/'inputs.csv').write_text('# header\n'+'\n'.join(input_rows)+'\nN 6,655')
            result,_=feedback.summarize(session)
            self.assertFalse(result['telemetry_available']);self.assertEqual(result['input_rows'],6)
            self.assertEqual(result['attack_edges'],[dict(sequence=2,action='primary')])
            self.assertEqual(result['camera_input']['moving_look_rows'],2)
            self.assertEqual(result['camera_input']['duplicate_movement_rows'],1)
            self.assertEqual(result['camera_input']['moving_look_windows'],[dict(start=3,end=4,frames=2)])
            columns=['sequence','host_elapsed_us','host_interval_us','retail_layout','game_vblank','game_updates',
                     'player','player_state','game_state','game_flags','health','grace','near_actor','near_health','incident','render_verify_mismatch']
            rows=[]
            for seq in range(6):
                # One three-VBlank simulation interval and one 80ms host gap;
                # unrelated quantities must be reported separately.
                values=[seq,seq*16667,80000 if seq==3 else 16667,1,seq,[0,0,1,1,1,2][seq],
                        0x80100000,0,0,0,300 if seq<3 else 290,110 if seq>=3 else 0,0x80101000,70,1 if seq>=4 else 0,0]
                rows.append(','.join(map(str,values)))
            (session/'telemetry.csv').write_text(','.join(columns)+'\n'+'\n'.join(rows)+'\n')
            result=feedback.write_report(session,root/'report')
            self.assertEqual(result['input_rows'],6)
            self.assertEqual(result['host_gaps'][0]['sequence'],3)
            self.assertEqual(result['update_gaps'][0]['vblanks'],3)
            self.assertEqual(result['damage'][0]['lost'],10)
            self.assertEqual(result['incidents'],[dict(sequence=4,incident=1)])
            self.assertEqual(result['render_verify_checks'],0)
            self.assertTrue((root/'report/timing.svg').exists())
            with self.assertRaises(FileExistsError):feedback.write_report(session,root/'report')


if __name__=='__main__':unittest.main()
